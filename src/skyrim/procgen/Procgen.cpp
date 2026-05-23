#include "skyrim/procgen/Procgen.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <random>
#include <unordered_map>
#include <vector>

#include <SKSE/Interfaces.h>

#include "skyrim/CoSave.h"  // central co-save dispatcher (one SetUniqueID per plugin)
#include "util.h"           // MathUtil::Angle (DegreeToRadian), FormUtil::Parse

namespace skyrim::procgen {

namespace {

// CONFIG_FOLDER from CMakeLists.txt (currently "Template_Plugin"). Recipes live
// under config/procgen/, copied next to the DLL by the post-build step.
constexpr const char* kConfigFolder = "Template_Plugin";

// Which Generate* call produced a tracked entry, so the co-save rebuild path can
// re-run the right one (research §5.2 strategy B).
enum class Kind : std::uint8_t { kInterior = 0, kStructure = 1 };

// ---- Tracked-room registry (research §5: track refs so we can clear/rearrange).
// The non-handle fields ({kind, recipeJson, origin, baseYaw, seed, cell/world
// plugin FormID}) are ALSO the co-save record: OnSave writes them, RebuildStaged
// reconstructs an identical entry from them. The ObjectRefHandles are session-
// local and are NEVER serialized (they hold dynamic 0xFF ids; research §5.2). --
struct GeneratedRoom {
    Kind kind = Kind::kInterior;
    std::string templateId;
    std::string recipeJson;        // self-contained recipe (rebuild source of truth)
    std::uint32_t seed = 1337;     // deterministic clutter scatter seed
    RE::NiPoint3 origin{};         // ABSOLUTE world-space room origin (as placed)
    float baseYaw = 0.f;           // anchor yaw applied to local offsets (radians)
    RE::FormID cellFormID = 0;     // parent cell plugin FormID (0 = exterior/unknown)
    RE::FormID worldFormID = 0;    // worldspace plugin FormID (0 = interior)
    std::vector<RE::ObjectRefHandle> shellHandles;
    std::vector<RE::ObjectRefHandle> furnitureHandles;  // parallel to furnitureSpecs
    std::vector<RE::ObjectRefHandle> otherHandles;      // lights + clutter
    std::vector<PieceSpec> furnitureSpecs;              // for deterministic rearrange
    int rearrangeCount = 0;        // how many times this room has been rearranged
};

// The registry is touched on the main thread (Generate*/Rearrange/Clear) AND read
// on the serialization thread (OnSave). Guard it (mirrors ProcgenNpc's g_mutex).
std::mutex g_mutex;
std::unordered_map<std::string, GeneratedRoom>& Rooms() {
    static std::unordered_map<std::string, GeneratedRoom> rooms;
    return rooms;
}
std::string& LastRoomKey() {
    static std::string key;
    return key;
}

// One co-save record read by OnLoad, awaiting a main-thread rebuild in
// RebuildStaged() (OnLoad runs off the main thread, so we cannot place here —
// research §5 "時機"). Only the recipe + transform + (already-remapped) plugin
// FormIDs are carried; the rebuild mints fresh refs.
struct StagedRoom {
    std::string key;
    Kind kind = Kind::kInterior;
    std::string recipeJson;
    std::uint32_t seed = 1337;
    RE::NiPoint3 origin{};
    float baseYaw = 0.f;
    RE::FormID cellFormID = 0;   // already ResolveFormID-remapped
    RE::FormID worldFormID = 0;  // already ResolveFormID-remapped
};
std::vector<StagedRoom> g_staged;  // main-thread-only (drained in RebuildStaged)

// Resolve a piece base form by EditorID or "0x..~Mod.esp" (SkyrimEntities uses
// the same convention; we duplicate the tiny EditorID/FormUtil split here to keep
// procgen self-contained). Returns null + a warning on miss (graceful, no crash).
RE::TESBoundObject* ResolveBase(const std::string& ref) {
    if (ref.empty()) return nullptr;
    if (ref.find('~') != std::string::npos) {
        if (auto* f = FormUtil::Parse::GetFormFromConfigString(ref)) {
            return f->As<RE::TESBoundObject>();
        }
        return nullptr;
    }
    if (ref.size() > 1 && ref[0] == '0' && (ref[1] == 'x' || ref[1] == 'X')) {
        try {
            const auto id = static_cast<RE::FormID>(std::stoul(ref, nullptr, 16));
            if (auto* f = RE::TESForm::LookupByID(id)) return f->As<RE::TESBoundObject>();
        } catch (...) {
        }
        return nullptr;
    }
    return RE::TESForm::LookupByEditorID<RE::TESBoundObject>(ref);
}

RE::hkpMotion::MotionType MotionFromString(const std::string& m) {
    if (m == "dynamic") return RE::hkpMotion::MotionType::kDynamic;
    if (m == "keyframed") return RE::hkpMotion::MotionType::kKeyframed;
    return RE::hkpMotion::MotionType::kFixed;  // default: pinned (research §4.3)
}

// Rotate a local-frame offset around Z by the anchor yaw, then translate by the
// origin -> world position (research §2.3 step 2: worldPos = O + Rz(theta)*local).
// Uses plain sin/cos to match NpcGenerator.cpp:36-39 (forward-vector convention:
// +Y is "forward" at yaw 0, matching Skyrim's actor facing).
RE::NiPoint3 LocalToWorld(const RE::NiPoint3& origin, float yaw, float lx, float ly, float lz) {
    const float s = std::sin(yaw);
    const float c = std::cos(yaw);
    RE::NiPoint3 world = origin;
    // Standard Z rotation; with Skyrim's convention (+Y forward), rotating the
    // local (x,y) by the anchor yaw keeps the room aligned to the caster facing.
    world.x += lx * c + ly * s;
    world.y += -lx * s + ly * c;
    world.z += lz;
    return world;
}

// Place one piece and return its handle. Applies ground-anchoring when requested
// (research §2 / EXTERIOR §2), sets the motion type, and forcePersist=false so
// the engine can reclaim the ref when the cell detaches (research §5.2 strategy B).
RE::ObjectRefHandle PlacePiece(const PieceSpec& p, const RE::NiPoint3& origin, float baseYaw,
                               RE::TESObjectCELL* cell, RE::TESWorldSpace* world) {
    auto* base = ResolveBase(p.base);
    if (!base) {
        SKSE::log::warn("Procgen: piece base '{}' did not resolve (skipped)", p.base);
        return RE::ObjectRefHandle();
    }

    RE::NiPoint3 pos = LocalToWorld(origin, baseYaw, p.localX, p.localY, p.localZ);

    // Ground-anchoring: snap Z to the land height under (x,y) (EXTERIOR §2). Only
    // valid in an exterior worldspace; GetLandHeight no-ops in interiors.
    if (p.anchorGround && world) {
        if (auto* tes = RE::TES::GetSingleton()) {
            float h = 0.f;
            RE::NiPoint3 probe{ pos.x, pos.y, 0.f };
            if (tes->GetLandHeight(probe, h)) {
                pos.z = h + p.localZ;  // localZ becomes an offset above ground
            }
        }
    }

    const float yawWorld = baseYaw + MathUtil::Angle::DegreeToRadian(p.rotZDeg);
    RE::NiPoint3 rot{ MathUtil::Angle::DegreeToRadian(p.rotXDeg),
                      MathUtil::Angle::DegreeToRadian(p.rotYDeg), yawWorld };

    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) {
        SKSE::log::error("Procgen: no TESDataHandler");
        return RE::ObjectRefHandle();
    }

    auto handle = dataHandler->CreateReferenceAtLocation(
        base, pos, rot, cell, world, nullptr, nullptr, RE::ObjectRefHandle(),
        /*forcePersist=*/false, /*a_arg11=*/true);

    if (auto refr = handle.get()) {
        refr->SetMotionType(MotionFromString(p.motion), true);
        SKSE::log::info("Procgen: placed '{}' -> {:X} at ({:.0f},{:.0f},{:.0f})",
                        p.base, refr->GetFormID(), pos.x, pos.y, pos.z);
    } else {
        SKSE::log::warn("Procgen: CreateReferenceAtLocation('{}') returned null", p.base);
    }
    return handle;
}

// Compute the room origin: anchorDistance units in front of the caster (forward
// vector, NpcGenerator.cpp:36-39). Returns origin + the yaw used.
void ComputeAnchor(RE::TESObjectREFR* anchor, float distance, RE::NiPoint3& outOrigin,
                   float& outYaw) {
    outYaw = anchor->data.angle.z;
    RE::NiPoint3 fwd{ std::sin(outYaw), std::cos(outYaw), 0.f };
    outOrigin = anchor->GetPosition();
    outOrigin.x += fwd.x * distance;
    outOrigin.y += fwd.y * distance;
}

// Place a list of pieces, appending handles to `out`. Shared by shell/furniture/
// lights. Furniture also records the spec parallel to its handle for rearrange.
void PlaceList(const std::vector<PieceSpec>& specs, const RE::NiPoint3& origin, float yaw,
               RE::TESObjectCELL* cell, RE::TESWorldSpace* world,
               std::vector<RE::ObjectRefHandle>& out, int& placedCount) {
    for (const auto& p : specs) {
        auto h = PlacePiece(p, origin, yaw, cell, world);
        if (h.get()) {
            out.push_back(h);
            ++placedCount;
        }
    }
}

// Scatter clutter pieces deterministically inside their authored AABB (research
// §3.4 / §8 clutter shape). Each clutter spec carries a count, seed, and AABB.
void PlaceClutter(const std::vector<PieceSpec>& clutter, const nlohmann::json& clutterDoc,
                  const RE::NiPoint3& origin, float yaw, RE::TESObjectCELL* cell,
                  RE::TESWorldSpace* world, std::vector<RE::ObjectRefHandle>& out,
                  int& placedCount) {
    // clutterDoc is the raw "clutter" array so we can read scatter_aabb/count/seed
    // without bloating PieceSpec; clutter[i] mirrors clutterDoc[i] base/motion.
    for (std::size_t i = 0; i < clutter.size() && i < clutterDoc.size(); ++i) {
        const auto& spec = clutter[i];
        const auto& jc = clutterDoc[i];
        int count = jc.value("count", 1);
        std::uint32_t seed = jc.value("seed", 1337u);
        RE::NiPoint3 lo{ 0, 0, 0 }, hi{ 0, 0, 0 };
        if (jc.contains("scatter_aabb") && jc["scatter_aabb"].is_array() &&
            jc["scatter_aabb"].size() == 2) {
            const auto& a = jc["scatter_aabb"][0];
            const auto& b = jc["scatter_aabb"][1];
            if (a.is_array() && a.size() == 3 && b.is_array() && b.size() == 3) {
                lo = { a[0].get<float>(), a[1].get<float>(), a[2].get<float>() };
                hi = { b[0].get<float>(), b[1].get<float>(), b[2].get<float>() };
            }
        }
        std::mt19937 rng(seed);  // deterministic (research §5.3 / SPEC §8)
        std::uniform_real_distribution<float> dx(lo.x, hi.x);
        std::uniform_real_distribution<float> dy(lo.y, hi.y);
        std::uniform_real_distribution<float> dz(lo.z, hi.z);
        std::uniform_real_distribution<float> dr(0.f, 360.f);
        for (int n = 0; n < count; ++n) {
            PieceSpec p = spec;
            p.localX = dx(rng);
            p.localY = dy(rng);
            p.localZ = dz(rng);
            p.rotZDeg = dr(rng);
            auto h = PlacePiece(p, origin, yaw, cell, world);
            if (h.get()) {
                out.push_back(h);
                ++placedCount;
            }
        }
    }
}

void DropRoom(GeneratedRoom& room) {
    auto disableAll = [](std::vector<RE::ObjectRefHandle>& v) {
        for (auto& h : v) {
            if (auto refr = h.get()) {
                refr->Disable();
                refr->SetDelete(true);
            }
        }
        v.clear();
    };
    disableAll(room.shellHandles);
    disableAll(room.furnitureHandles);
    disableAll(room.otherHandles);
    room.furnitureSpecs.clear();
}

// Capture the parent cell / worldspace *plugin* FormIDs for the co-save (research
// §5.2: store a remappable plugin FormID, NEVER a dynamic 0xFF id). 0 means
// interior-without-stable-cell / exterior-without-worldspace as appropriate.
void CaptureLocation(GeneratedRoom& room, RE::TESObjectCELL* cell, RE::TESWorldSpace* world) {
    if (cell && cell->GetFormID() < 0xFF000000) room.cellFormID = cell->GetFormID();
    if (world && world->GetFormID() < 0xFF000000) room.worldFormID = world->GetFormID();
}

// Core interior placement at an EXPLICIT absolute origin/yaw (no anchor math).
// Shared by GenerateInterior (origin from anchor) and RebuildStaged (origin from
// the co-save). Fills the handle vectors + furnitureSpecs on `room`; returns the
// ref count placed. clutterDoc carries the scatter params (count/seed/AABB); when
// empty, clutter falls back to its authored single positions (research §3.4).
int PlaceInterior(const Recipe& recipe, const RE::NiPoint3& origin, float yaw,
                  RE::TESObjectCELL* cell, RE::TESWorldSpace* world,
                  const nlohmann::json& clutterDoc, GeneratedRoom& room) {
    int placed = 0;
    // research §7.2 order: floors/walls/roof (shell) -> furniture -> lights -> clutter.
    PlaceList(recipe.shell, origin, yaw, cell, world, room.shellHandles, placed);
    for (const auto& f : recipe.furniture) {
        auto h = PlacePiece(f, origin, yaw, cell, world);
        if (h.get()) {
            room.furnitureHandles.push_back(h);
            room.furnitureSpecs.push_back(f);
            ++placed;
        }
    }
    PlaceList(recipe.lights, origin, yaw, cell, world, room.otherHandles, placed);
    // Clutter: if the raw JSON array is supplied (JSON-driven adapter path / the
    // co-save-stored recipe) we scatter inside each entry's AABB (count/seed);
    // otherwise (bare spell path) we place each clutter spec once at its authored
    // localX/Y.
    if (!recipe.clutter.empty()) {
        if (clutterDoc.is_array() && !clutterDoc.empty()) {
            PlaceClutter(recipe.clutter, clutterDoc, origin, yaw, cell, world,
                         room.otherHandles, placed);
        } else {
            for (const auto& c : recipe.clutter) {
                auto h = PlacePiece(c, origin, yaw, cell, world);
                if (h.get()) {
                    room.otherHandles.push_back(h);
                    ++placed;
                }
            }
        }
    }
    return placed;
}

// Core structure placement at an EXPLICIT absolute origin/yaw. Shared by
// GenerateStructure and RebuildStaged. Note: flatten_to_max land sampling is done
// by the caller (it shifts origin.z) so the SAME origin is reused on rebuild
// rather than re-sampling (the saved origin already baked the flatten result).
int PlaceStructure(const Recipe& recipe, const RE::NiPoint3& origin, float yaw,
                   RE::TESObjectCELL* cell, RE::TESWorldSpace* world, GeneratedRoom& room) {
    int placed = 0;
    PlaceList(recipe.shell, origin, yaw, cell, world, room.shellHandles, placed);
    return placed;
}

}  // namespace

// ---------------------------------------------------------------------------
// Recipe parsing
// ---------------------------------------------------------------------------

namespace {

float jnum(const nlohmann::json& j, const char* k, float fallback) {
    return (j.contains(k) && j[k].is_number()) ? j[k].get<float>() : fallback;
}

// Parse one piece entry. Supports both the interior shape ({base, grid:[gx,gy],
// z, rot_deg:[x,y,z], motion}) and the exterior shape ({piece, at:[x,y,z],
// rot_z, anchor_ground, motion}). gridStep scales interior grid coords -> units.
PieceSpec ParsePiece(const nlohmann::json& j, float gridStep) {
    PieceSpec p;
    p.base = j.value("base", j.value("piece", std::string{}));
    p.motion = j.value("motion", std::string{"fixed"});
    p.slot = j.value("slot", std::string{});
    p.anchorGround = j.value("anchor_ground", false);

    // Position: interior grid [gx,gy] + z, OR explicit at/pos [x,y,z].
    if (j.contains("grid") && j["grid"].is_array() && j["grid"].size() >= 2) {
        p.localX = j["grid"][0].get<float>() * gridStep;
        p.localY = j["grid"][1].get<float>() * gridStep;
        p.localZ = jnum(j, "z", 0.f);
    } else {
        const nlohmann::json* at = nullptr;
        if (j.contains("at")) at = &j["at"];
        else if (j.contains("pos")) at = &j["pos"];
        if (at && at->is_array() && at->size() >= 3) {
            p.localX = (*at)[0].get<float>();
            p.localY = (*at)[1].get<float>();
            p.localZ = (*at)[2].get<float>();
        }
    }

    // Rotation: rot_deg:[x,y,z] (interior) OR rot_z scalar (exterior).
    if (j.contains("rot_deg") && j["rot_deg"].is_array() && j["rot_deg"].size() >= 3) {
        p.rotXDeg = j["rot_deg"][0].get<float>();
        p.rotYDeg = j["rot_deg"][1].get<float>();
        p.rotZDeg = j["rot_deg"][2].get<float>();
    } else {
        p.rotZDeg = jnum(j, "rot_z", 0.f);
    }
    return p;
}

// Expand an exterior piece's optional {repeat:{axis,step,count}} into N pieces
// stepping along the named local axis (research EXTERIOR §9 repeat shape).
void ExpandRepeat(const nlohmann::json& j, const PieceSpec& base,
                  std::vector<PieceSpec>& out) {
    if (!j.contains("repeat") || !j["repeat"].is_object()) {
        out.push_back(base);
        return;
    }
    const auto& r = j["repeat"];
    std::string axis = r.value("axis", std::string{"x"});
    float step = jnum(r, "step", 0.f);
    int count = r.value("count", 1);
    for (int i = 0; i < count; ++i) {
        PieceSpec p = base;
        if (axis == "x") p.localX += step * i;
        else if (axis == "y") p.localY += step * i;
        else p.localZ += step * i;
        out.push_back(p);
    }
}

}  // namespace

bool ParseRecipe(const nlohmann::json& doc, Recipe& out) {
    if (!doc.is_object()) {
        SKSE::log::error("Procgen: recipe is not a JSON object");
        return false;
    }
    out.templateId = doc.value("template_id", std::string{});
    out.gridStep = jnum(doc, "grid_step", 256.f);
    out.seed = doc.value("seed", 1337u);

    if (doc.contains("anchor") && doc["anchor"].is_object()) {
        out.anchorDistance = jnum(doc["anchor"], "distance", 300.f);
        out.anchorDistance = jnum(doc["anchor"], "forward", out.anchorDistance);
    }

    // Exterior fields (footprint + ground rule).
    if (doc.contains("footprint") && doc["footprint"].is_object()) {
        const auto& fp = doc["footprint"];
        out.cellSize = jnum(fp, "cell_size", 1024.f);
        if (fp.contains("cells") && fp["cells"].is_array() && fp["cells"].size() >= 2) {
            out.footprintCellsX = fp["cells"][0].get<int>();
            out.footprintCellsY = fp["cells"][1].get<int>();
        }
    }
    if (doc.contains("ground_rule") && doc["ground_rule"].is_object()) {
        out.groundRule = doc["ground_rule"].value("mode", std::string{"none"});
    }
    out.groundRule = doc.value("ground", out.groundRule);

    // Shell: interior "shell" array OR exterior "pieces" array (with repeat).
    out.shell.clear();
    auto parseShellArray = [&](const nlohmann::json& arr, bool exterior) {
        for (const auto& j : arr) {
            PieceSpec base = ParsePiece(j, out.gridStep);
            if (exterior) ExpandRepeat(j, base, out.shell);
            else out.shell.push_back(base);
        }
    };
    if (doc.contains("shell") && doc["shell"].is_array()) {
        parseShellArray(doc["shell"], false);
    } else if (doc.contains("pieces") && doc["pieces"].is_array()) {
        parseShellArray(doc["pieces"], true);
    }

    // Interior-only lists.
    out.furniture.clear();
    if (doc.contains("furniture_slots") && doc["furniture_slots"].is_array()) {
        for (const auto& j : doc["furniture_slots"]) {
            out.furniture.push_back(ParsePiece(j, out.gridStep));
        }
    }
    out.lights.clear();
    if (doc.contains("lights") && doc["lights"].is_array()) {
        for (const auto& j : doc["lights"]) {
            out.lights.push_back(ParsePiece(j, out.gridStep));
        }
    }
    out.clutter.clear();
    if (doc.contains("clutter") && doc["clutter"].is_array()) {
        for (const auto& j : doc["clutter"]) {
            out.clutter.push_back(ParsePiece(j, out.gridStep));
        }
    }

    if (out.shell.empty() && out.furniture.empty()) {
        SKSE::log::warn("Procgen: recipe '{}' has no shell/furniture pieces", out.templateId);
    }
    return true;
}

bool LoadRecipeFile(const std::string& fileName, Recipe& out) {
    namespace fs = std::filesystem;
    fs::path path = fs::path("Data") / "SKSE" / "Plugins" / kConfigFolder / "procgen" / fileName;
    if (!fs::exists(path)) {
        // Fallback: a Data-relative procgen dir.
        path = fs::path("Data") / "SKSE" / "Plugins" / "procgen" / fileName;
    }
    std::ifstream in(path);
    if (!in) {
        SKSE::log::error("Procgen: cannot open recipe file: {}", path.string());
        return false;
    }
    nlohmann::json doc;
    try {
        in >> doc;
    } catch (const std::exception& e) {
        SKSE::log::error("Procgen: JSON parse error in {}: {}", path.string(), e.what());
        return false;
    }
    SKSE::log::info("Procgen: loaded recipe '{}' from {}",
                    doc.value("template_id", "?"), path.string());
    return ParseRecipe(doc, out);
}

// ---------------------------------------------------------------------------
// Recipe -> self-contained JSON (for the co-save). The emitted doc round-trips
// through ParseRecipe to an equivalent Recipe (research §5.2: store the food
// recipe, not the placed refs). Pieces are written in the canonical `at`/`rot_deg`
// form using the ALREADY-SCALED local units in PieceSpec, so we set grid_step=1
// to avoid re-scaling on the next ParseRecipe (interior grid math is pre-applied).
// ---------------------------------------------------------------------------

nlohmann::json RecipeToJson(const Recipe& recipe) {
    auto pieceToJson = [](const PieceSpec& p) {
        nlohmann::json j;
        j["base"] = p.base;
        j["at"] = { p.localX, p.localY, p.localZ };
        j["rot_deg"] = { p.rotXDeg, p.rotYDeg, p.rotZDeg };
        j["motion"] = p.motion;
        if (!p.slot.empty()) j["slot"] = p.slot;
        if (p.anchorGround) j["anchor_ground"] = true;
        return j;
    };
    auto listToJson = [&](const std::vector<PieceSpec>& v) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& p : v) arr.push_back(pieceToJson(p));
        return arr;
    };

    nlohmann::json doc;
    doc["template_id"] = recipe.templateId;
    doc["grid_step"] = 1.0f;  // local units are pre-scaled (see header comment)
    doc["seed"] = recipe.seed;
    doc["anchor"] = { { "distance", recipe.anchorDistance } };
    doc["ground"] = recipe.groundRule;
    doc["footprint"] = { { "cell_size", recipe.cellSize },
                         { "cells", { recipe.footprintCellsX, recipe.footprintCellsY } } };
    doc["shell"] = listToJson(recipe.shell);
    if (!recipe.furniture.empty()) doc["furniture_slots"] = listToJson(recipe.furniture);
    if (!recipe.lights.empty()) doc["lights"] = listToJson(recipe.lights);
    if (!recipe.clutter.empty()) doc["clutter"] = listToJson(recipe.clutter);
    return doc;
}

// ---------------------------------------------------------------------------
// Generation
// ---------------------------------------------------------------------------

int GenerateInterior(const Recipe& recipe, RE::TESObjectREFR* anchor,
                     const std::string& persistKey, const nlohmann::json& clutterDoc) {
    if (!anchor) {
        SKSE::log::warn("Procgen: GenerateInterior with null anchor");
        return 0;
    }
    auto* cell = anchor->GetParentCell();
    auto* world = anchor->GetWorldspace();

    RE::NiPoint3 origin{};
    float yaw = 0.f;
    ComputeAnchor(anchor, recipe.anchorDistance, origin, yaw);

    const std::string key = persistKey.empty() ? recipe.templateId : persistKey;
    // Clear any prior room under this key so a re-cast doesn't stack/leak.
    ClearGenerated(key);

    GeneratedRoom room;
    room.kind = Kind::kInterior;
    room.templateId = recipe.templateId;
    room.seed = recipe.seed;
    room.origin = origin;
    room.baseYaw = yaw;
    CaptureLocation(room, cell, world);

    // Store a self-contained recipe for the co-save rebuild. If a raw clutter doc
    // was supplied (adapter scatter path), fold it into the stored recipe so the
    // SAME scatter (count/seed/AABB) is reproduced on reload (research §5.2/§5.3).
    nlohmann::json recipeDoc = RecipeToJson(recipe);
    if (clutterDoc.is_array() && !clutterDoc.empty()) recipeDoc["clutter"] = clutterDoc;
    room.recipeJson = recipeDoc.dump();

    const int placed = PlaceInterior(recipe, origin, yaw, cell, world, clutterDoc, room);

    {
        std::scoped_lock lock(g_mutex);
        Rooms()[key] = std::move(room);
    }
    LastRoomKey() = key;
    SKSE::log::info("Procgen: GenerateInterior '{}' key='{}' placed {} refs (co-saved)",
                    recipe.templateId, key, placed);
    return placed;
}

int GenerateStructure(const Recipe& recipe, RE::TESObjectREFR* anchor,
                      const std::string& persistKey) {
    if (!anchor) {
        SKSE::log::warn("Procgen: GenerateStructure with null anchor");
        return 0;
    }
    auto* world = anchor->GetWorldspace();
    if (!world) {
        // EXTERIOR §8 step 1: exterior-only. Abort in interiors.
        SKSE::log::warn("Procgen: GenerateStructure aborted (no worldspace; interior?)");
        return 0;
    }
    auto* cell = anchor->GetParentCell();

    RE::NiPoint3 origin{};
    float yaw = 0.f;
    ComputeAnchor(anchor, recipe.anchorDistance, origin, yaw);

    // flatten_to_max (EXTERIOR §2/§8 step 2): sample land height across the
    // footprint, take the max, and place the whole structure on that plane. Only
    // pieces flagged anchor_ground (foundations) snap individually. We bake the
    // result into origin.z and STORE that absolute origin, so the reload rebuild
    // reuses it directly rather than re-sampling (land sampling needs the cell
    // loaded; the stored origin is self-sufficient — research §5.2).
    if (recipe.groundRule == "flatten_to_max") {
        if (auto* tes = RE::TES::GetSingleton()) {
            float zMax = origin.z;
            const float half = recipe.cellSize * 0.5f;
            for (int gx = -recipe.footprintCellsX; gx <= recipe.footprintCellsX; ++gx) {
                for (int gy = -recipe.footprintCellsY; gy <= recipe.footprintCellsY; ++gy) {
                    RE::NiPoint3 probe{ origin.x + gx * half, origin.y + gy * half, 0.f };
                    float h = 0.f;
                    if (tes->GetLandHeight(probe, h) && h > zMax) zMax = h;
                }
            }
            origin.z = zMax;
            SKSE::log::info("Procgen: flatten_to_max -> base z {:.0f}", zMax);
        }
    }

    const std::string key = persistKey.empty() ? recipe.templateId : persistKey;
    ClearGenerated(key);

    GeneratedRoom room;
    room.kind = Kind::kStructure;
    room.templateId = recipe.templateId;
    room.seed = recipe.seed;
    room.origin = origin;
    room.baseYaw = yaw;
    CaptureLocation(room, cell, world);
    // Store the recipe with ground_rule forced to "none": origin.z already baked
    // the flatten result, so the rebuild must NOT flatten again (it would re-add
    // the offset). Per-piece anchor_ground snapping is preserved (it re-snaps to
    // current land, which is correct on rebuild).
    nlohmann::json recipeDoc = RecipeToJson(recipe);
    recipeDoc["ground"] = "none";
    room.recipeJson = recipeDoc.dump();

    const int placed = PlaceStructure(recipe, origin, yaw, cell, world, room);

    {
        std::scoped_lock lock(g_mutex);
        Rooms()[key] = std::move(room);
    }
    LastRoomKey() = key;
    SKSE::log::info("Procgen: GenerateStructure '{}' key='{}' placed {} refs (co-saved)",
                    recipe.templateId, key, placed);
    return placed;
}

int RearrangeFurnishings(const std::string& persistKey) {
    // Hold the registry lock for the whole pass: we keep a GeneratedRoom& live
    // and OnSave may read the registry on the serialization thread (g_mutex).
    std::scoped_lock lock(g_mutex);
    const std::string key = persistKey.empty() ? LastRoomKey() : persistKey;
    auto it = Rooms().find(key);
    if (key.empty() || it == Rooms().end()) {
        SKSE::log::warn("Procgen: RearrangeFurnishings — no room tracked (key='{}')", key);
        return 0;
    }
    GeneratedRoom& room = it->second;
    if (room.furnitureSpecs.empty()) {
        SKSE::log::warn("Procgen: RearrangeFurnishings — room '{}' has no furniture", key);
        return 0;
    }

    // Deterministic shuffle of furniture positions: seed by room id + call count
    // so each cast yields a different but reproducible layout (research §5.3
    // determinism). We re-place existing refs (move/rotate) where possible, and
    // for any handle that went stale we re-create from the stored spec.
    ++room.rearrangeCount;
    std::mt19937 rng(static_cast<std::uint32_t>(
        std::hash<std::string>{}(key) + room.rearrangeCount * 2654435761u));

    // Collect the slot transforms and shuffle their assignment so furniture swaps
    // positions (the "變更屋子擺設" request — move/rotate/replace).
    std::vector<std::size_t> order(room.furnitureSpecs.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::shuffle(order.begin(), order.end(), rng);
    std::uniform_real_distribution<float> jitterRot(-30.f, 30.f);

    // Recover cell/world from the first live furniture ref (kept simple/safe).
    RE::TESObjectCELL* cell = nullptr;
    RE::TESWorldSpace* world = nullptr;
    for (auto& h : room.furnitureHandles) {
        if (auto refr = h.get()) {
            cell = refr->GetParentCell();
            world = refr->GetWorldspace();
            break;
        }
    }

    int moved = 0;
    for (std::size_t i = 0; i < room.furnitureHandles.size(); ++i) {
        const PieceSpec& targetSpec = room.furnitureSpecs[order[i % order.size()]];
        RE::NiPoint3 newPos =
            LocalToWorld(room.origin, room.baseYaw, targetSpec.localX, targetSpec.localY,
                         targetSpec.localZ);
        const float yawWorld = room.baseYaw +
            MathUtil::Angle::DegreeToRadian(targetSpec.rotZDeg + jitterRot(rng));
        RE::NiPoint3 newAngle{ 0.f, 0.f, yawWorld };

        if (auto refr = room.furnitureHandles[i].get()) {
            refr->SetPosition(newPos);
            refr->SetAngle(newAngle);
            ++moved;
        } else {
            // Stale handle: re-create from spec (research §5 "rebuild from recipe").
            PieceSpec p = room.furnitureSpecs[i];
            p.localX = targetSpec.localX;
            p.localY = targetSpec.localY;
            p.localZ = targetSpec.localZ;
            p.rotZDeg = targetSpec.rotZDeg;
            auto h = PlacePiece(p, room.origin, room.baseYaw, cell, world);
            if (h.get()) {
                room.furnitureHandles[i] = h;
                ++moved;
            }
        }
    }
    SKSE::log::info("Procgen: RearrangeFurnishings '{}' moved {} furniture refs (pass {})", key,
                    moved, room.rearrangeCount);
    return moved;
}

void ClearGenerated(const std::string& persistKey) {
    // Erasing from Rooms() also drops the corresponding co-save record: OnSave
    // serializes the live registry, so a cleared room is not written to the next
    // save and therefore does not reappear on the following reload (research §5).
    std::scoped_lock lock(g_mutex);
    if (persistKey.empty()) {
        for (auto& [k, room] : Rooms()) DropRoom(room);
        Rooms().clear();
        LastRoomKey().clear();
        SKSE::log::info("Procgen: cleared ALL generated rooms");
        return;
    }
    auto it = Rooms().find(persistKey);
    if (it != Rooms().end()) {
        DropRoom(it->second);
        Rooms().erase(it);
        if (LastRoomKey() == persistKey) LastRoomKey().clear();
        SKSE::log::info("Procgen: cleared room '{}'", persistKey);
    }
}

// ---------------------------------------------------------------------------
// Co-save persistence (research §5.2 strategy B). Record type 'PRGN', registered
// with the central skyrim::cosave dispatcher (no SetUniqueID of our own — CoSave.h
// on the one-SetUniqueID-per-plugin limit). We store the recipe + absolute
// origin/yaw/seed + cell/world *plugin* FormID and rebuild on load; we NEVER store
// the dynamic 0xFF ref FormIDs (those are session-local and unstable).
// ---------------------------------------------------------------------------

namespace {

// Length-prefixed string I/O (mirrors ProcgenNpc's WriteString/ReadString,
// including the 1 MiB sanity cap so a corrupt length can't OOM on read).
bool WriteString(SKSE::SerializationInterface* intfc, const std::string& s) {
    const auto len = static_cast<std::uint32_t>(s.size());
    if (!intfc->WriteRecordData(len)) return false;
    if (len == 0) return true;
    return intfc->WriteRecordData(s.data(), len);
}

bool ReadString(SKSE::SerializationInterface* intfc, std::string& out) {
    std::uint32_t len = 0;
    if (intfc->ReadRecordData(len) == 0) return false;
    out.clear();
    if (len == 0) return true;
    if (len > (1u << 20)) {  // 1 MiB sanity cap — a corrupt length must not OOM
        SKSE::log::error("Procgen: refusing absurd string length {}", len);
        return false;
    }
    out.resize(len);
    return intfc->ReadRecordData(out.data(), len) == len;
}

}  // namespace

void OnSave(SKSE::SerializationInterface* intfc) {
    std::scoped_lock lock(g_mutex);
    auto& reg = Rooms();

    if (!intfc->OpenRecord(kRecordType, kRecordVersion)) {
        SKSE::log::error("Procgen: OnSave OpenRecord failed");
        return;
    }
    const auto count = static_cast<std::uint32_t>(reg.size());
    intfc->WriteRecordData(count);

    for (auto& [key, room] : reg) {
        WriteString(intfc, key);
        const auto kind = static_cast<std::uint8_t>(room.kind);
        intfc->WriteRecordData(kind);
        intfc->WriteRecordData(room.seed);
        intfc->WriteRecordData(room.origin);   // absolute world origin (as placed)
        intfc->WriteRecordData(room.baseYaw);
        // EXISTING plugin FormIDs only — ResolveFormID remaps them on load. NEVER
        // the 0xFF dynamic ref ids (research §5.2).
        intfc->WriteRecordData(room.cellFormID);
        intfc->WriteRecordData(room.worldFormID);
        WriteString(intfc, room.recipeJson);
    }
    SKSE::log::info("Procgen: OnSave wrote {} generated room/structure recipe(s)", count);
}

void OnLoad(SKSE::SerializationInterface* intfc, std::uint32_t version, std::uint32_t /*length*/) {
    // Serialization thread: stage only, never place here (research §5 "時機").
    // The dispatcher already matched our record by type, so we read just its
    // payload and defer the rebuild to RebuildStaged() on kPostLoadGame.
    g_staged.clear();

    if (version != kRecordVersion) {
        SKSE::log::warn("Procgen: OnLoad record version {} != {} (ignored)", version,
                        kRecordVersion);
        return;
    }
    std::uint32_t count = 0;
    if (intfc->ReadRecordData(count) == 0) {
        SKSE::log::error("Procgen: OnLoad failed to read count");
        return;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        StagedRoom s;
        if (!ReadString(intfc, s.key)) break;
        std::uint8_t kind = 0;
        intfc->ReadRecordData(kind);
        s.kind = static_cast<Kind>(kind);
        intfc->ReadRecordData(s.seed);
        intfc->ReadRecordData(s.origin);
        intfc->ReadRecordData(s.baseYaw);
        intfc->ReadRecordData(s.cellFormID);
        intfc->ReadRecordData(s.worldFormID);
        if (!ReadString(intfc, s.recipeJson)) break;

        // Remap the EXISTING plugin FormIDs to this load order (research §5.3 /
        // SerializationInterface::ResolveFormID). Only the cell/world plugin ids
        // are remapped; the recipe's piece bases are EditorID/plugin strings
        // re-resolved at rebuild time.
        if (s.cellFormID) {
            RE::FormID rc = s.cellFormID;
            if (intfc->ResolveFormID(s.cellFormID, rc)) s.cellFormID = rc;
        }
        if (s.worldFormID) {
            RE::FormID rw = s.worldFormID;
            if (intfc->ResolveFormID(s.worldFormID, rw)) s.worldFormID = rw;
        }
        g_staged.push_back(std::move(s));
    }
    SKSE::log::info("Procgen: OnLoad staged {} room/structure recipe(s) for rebuild",
                    g_staged.size());
}

void OnRevert(SKSE::SerializationInterface*) {
    // Revert: drop the in-memory registry. The placed refs belong to the outgoing
    // save and are torn down by the engine (research §5). We do NOT DropRoom()
    // (that would touch RE:: on the serialization thread); the engine reclaims the
    // forcePersist=false refs as cells detach.
    std::scoped_lock lock(g_mutex);
    Rooms().clear();
    LastRoomKey().clear();
    g_staged.clear();
    SKSE::log::info("Procgen: OnRevert cleared the in-memory registry");
}

bool Register() {
    // Register a 'PRGN' handler with the central dispatcher (CoSave.h: one
    // SetUniqueID per plugin — must not call SetUniqueID/SetSaveCallback here).
    skyrim::cosave::AddHandler({ kRecordType, &OnSave, &OnLoad, &OnRevert });
    SKSE::log::info("Procgen: registered 'PRGN' co-save handler with dispatcher");
    return true;
}

void RebuildStaged() {
    // Drain the staged records into a local copy (main thread only). Each is
    // re-run through PlaceInterior/PlaceStructure at its STORED absolute origin so
    // the same room/castle reappears in the same place (research §5.2 strategy B).
    std::vector<StagedRoom> staged;
    staged.swap(g_staged);
    if (staged.empty()) return;

    {
        std::scoped_lock lock(g_mutex);
        Rooms().clear();  // the old session's refs are gone (research §5)
        LastRoomKey().clear();
    }

    std::size_t rebuilt = 0;
    for (auto& s : staged) {
        Recipe recipe;
        nlohmann::json doc;
        try {
            doc = nlohmann::json::parse(s.recipeJson);
        } catch (const std::exception& e) {
            SKSE::log::error("Procgen: RebuildStaged bad recipe for '{}': {}", s.key, e.what());
            continue;
        }
        if (!ParseRecipe(doc, recipe)) {
            SKSE::log::warn("Procgen: RebuildStaged could not parse recipe for '{}'", s.key);
            continue;
        }

        // Resolve the stored cell/worldspace plugin FormIDs back to live pointers
        // (null = let the engine pick by coordinates, as the original anchor path
        // did). Interiors carry a cell; exteriors a worldspace.
        RE::TESObjectCELL* cell = nullptr;
        RE::TESWorldSpace* world = nullptr;
        if (s.cellFormID) {
            if (auto* f = RE::TESForm::LookupByID(s.cellFormID)) cell = f->As<RE::TESObjectCELL>();
        }
        if (s.worldFormID) {
            if (auto* f = RE::TESForm::LookupByID(s.worldFormID)) {
                world = f->As<RE::TESWorldSpace>();
            }
        }

        GeneratedRoom room;
        room.kind = s.kind;
        room.templateId = recipe.templateId;
        room.seed = s.seed;
        room.origin = s.origin;
        room.baseYaw = s.baseYaw;
        room.cellFormID = s.cellFormID;
        room.worldFormID = s.worldFormID;
        room.recipeJson = s.recipeJson;  // keep the same self-contained recipe

        int placed = 0;
        if (s.kind == Kind::kStructure) {
            placed = PlaceStructure(recipe, s.origin, s.baseYaw, cell, world, room);
        } else {
            // The stored recipe already folds in any clutter scatter params, so we
            // pass its clutter array as the clutterDoc to reproduce the scatter.
            const nlohmann::json clutterDoc =
                doc.contains("clutter") ? doc["clutter"] : nlohmann::json::array();
            placed = PlaceInterior(recipe, s.origin, s.baseYaw, cell, world, clutterDoc, room);
        }

        {
            std::scoped_lock lock(g_mutex);
            Rooms()[s.key] = std::move(room);
            LastRoomKey() = s.key;
        }
        ++rebuilt;
        SKSE::log::info("Procgen: rebuilt '{}' ({} refs) at ({:.0f},{:.0f},{:.0f})", s.key, placed,
                        s.origin.x, s.origin.y, s.origin.z);
    }
    SKSE::log::info("Procgen: RebuildStaged rebuilt {}/{} rooms/structures", rebuilt, staged.size());
}

}  // namespace skyrim::procgen
