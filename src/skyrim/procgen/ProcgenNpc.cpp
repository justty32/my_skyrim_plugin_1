#include "skyrim/procgen/ProcgenNpc.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <SKSE/API.h>
#include <SKSE/Interfaces.h>

#include "util.h"  // FormUtil::Parse, Util::String

namespace skyrim::procgen::npc {

namespace {

// PLACEHOLDER (research §1.3 / §11 caveat): default template is a real vanilla
// generic NPC base. 0x0001327B is "Bandit" (Skyrim.esm) — a complete,
// fully-rigged actor base (race/skeleton/AI/voice/outfit), so Copy() yields an
// immediately-usable NPC without hand-filling the 12 components in research §1.2.
// TODO: pick a friendlier "citizen" template per use case; override per-recipe
// via the recipe's "template" field. Validate the FormID in-game.
constexpr RE::FormID kDefaultTemplate = 0x0001327B;  // Skyrim.esm Bandit

// One runtime-tracked generated NPC. We track the live ref by handle (never a
// raw pointer — research §5 step 2) and keep the recipe JSON verbatim so the
// load path can rebuild an identical instance. We also remember the resolved
// template's *plugin* FormID so the co-save can store a remappable id (NOT the
// 0xFF dynamic base/ref id).
struct TrackedNpc {
    std::string key;                  // stable string id (logical identity)
    RE::FormID templatePluginFormID;  // EXISTING plugin form, ResolveFormID-safe
    std::string recipeJson;           // full recipe (rebuild source of truth)
    RE::ObjectRefHandle ref;          // live placed ref (session-local)
    RE::NiPoint3 lastPos{};           // last known world position (for rebuild)
    RE::FormID cellFormID = 0;        // parent cell plugin FormID (0 = exterior/unknown)
    RE::FormID worldFormID = 0;       // worldspace plugin FormID (0 = interior)
};

// Registry guarded by a mutex: OnSave/OnLoad/OnRevert run on the serialization
// thread, Generate()/RebuildStaged() on the main thread (research THREADING).
std::mutex g_mutex;
std::unordered_map<std::string, TrackedNpc>& Registry() {
    static std::unordered_map<std::string, TrackedNpc> reg;
    return reg;
}

// Recipes read by OnLoad, awaiting a main-thread rebuild in RebuildStaged()
// (research §5 "時機": OnLoad runs off the main thread, so we cannot mint here).
struct StagedRecipe {
    std::string key;
    RE::FormID templatePluginFormID;  // already remapped by ResolveFormID
    std::string recipeJson;
    RE::NiPoint3 lastPos;
    RE::FormID cellFormID;
    RE::FormID worldFormID;
};
std::vector<StagedRecipe> g_staged;

std::uint32_t& AutoKeyCounter() {
    static std::uint32_t n = 0;
    return n;
}

// Resolve a recipe "template" string to a TESNPC + remember its plugin FormID.
// Accepts "0x..~Mod.esp" (FormUtil::Parse), bare "0x.." absolute id, or EditorID
// (mirrors SkyrimEntities::resolveForm so the vocabulary is consistent).
RE::TESNPC* ResolveTemplate(const std::string& ref, RE::FormID& outPluginFormID) {
    RE::TESForm* form = nullptr;
    if (!ref.empty()) {
        if (ref.find('~') != std::string::npos) {
            form = FormUtil::Parse::GetFormFromConfigString(ref);
        } else if (ref.size() > 1 && ref[0] == '0' && (ref[1] == 'x' || ref[1] == 'X')) {
            try {
                const auto id = static_cast<RE::FormID>(std::stoul(ref, nullptr, 16));
                form = RE::TESForm::LookupByID(id);
            } catch (...) {
            }
        } else {
            form = RE::TESForm::LookupByEditorID(ref);
        }
    }
    if (!form) {
        form = RE::TESForm::LookupByID(kDefaultTemplate);  // placeholder fallback
    }
    auto* npc = form ? form->As<RE::TESNPC>() : nullptr;
    // Only an EXISTING plugin form has a ResolveFormID-safe id. A dynamic 0xFF
    // template would be useless to store; in that (unexpected) case fall back to
    // the placeholder so the co-save stays remappable (research §3.3).
    if (npc && npc->GetFormID() < 0xFF000000) {
        outPluginFormID = npc->GetFormID();
    } else {
        outPluginFormID = kDefaultTemplate;
    }
    return npc;
}

// Apply the optional recipe overrides onto a freshly Copy()'d base (research
// §1.3 / §4: name, sex flag, level, essential flag). Appearance morph/tint is
// inherited from the template (research §4: Copy carries the baked face), which
// is the safe path; deep appearance procgen is left as a TODO.
void ApplyOverrides(RE::TESNPC* base, const nlohmann::json& recipe) {
    if (!base) return;

    const std::string name = recipe.value("name", std::string{});
    if (!name.empty()) {
        base->SetFullName(name.c_str());  // TESFullName::SetFullName
    }

    if (recipe.contains("sex") && recipe["sex"].is_string()) {
        const std::string sex = Util::String::ToLower(recipe["sex"].get<std::string>());
        const bool female = (sex == "female" || sex == "f");
        base->actorData.actorBaseFlags.set(female, RE::ACTOR_BASE_DATA::Flag::kFemale);
    }

    if (recipe.contains("essential") && recipe["essential"].is_boolean()) {
        base->actorData.actorBaseFlags.set(recipe["essential"].get<bool>(),
                                           RE::ACTOR_BASE_DATA::Flag::kEssential);
    }

    if (recipe.contains("level") && recipe["level"].is_number()) {
        const auto lvl = recipe["level"].get<int>();
        if (lvl > 0) {
            base->actorData.level = static_cast<std::uint16_t>(lvl);
        }
    }
}

// Core mint + place. `anchor` is where the ref is dropped. Returns the placed
// actor (null on failure). This is the SAME factory->Create()<TESNPC> + Copy +
// PlaceObjectAtMe recipe as NpcGenerator::SpawnNpc / SkyrimEntities::spawnCharacter
// (research §0/§1) — factored here so the generation core is reused, not copied.
RE::Actor* MintAndPlace(RE::TESNPC* templateNpc, const nlohmann::json& recipe,
                        RE::TESObjectREFR* anchor) {
    if (!anchor) {
        SKSE::log::error("ProcgenNpc: MintAndPlace with null anchor");
        return nullptr;
    }
    auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
    if (!templateNpc || !factory) {
        SKSE::log::error("ProcgenNpc: missing template NPC or factory");
        return nullptr;
    }

    auto* newBase = factory->Create()->As<RE::TESNPC>();  // mint a real TESNPC form
    if (!newBase) {
        SKSE::log::error("ProcgenNpc: factory returned null base");
        return nullptr;
    }
    newBase->Copy(templateNpc);   // inherit the complete, baked NPC (research §1.3)
    ApplyOverrides(newBase, recipe);

    auto spawned = anchor->PlaceObjectAtMe(newBase, false);  // temporary ref (research §5)
    auto* actor = spawned ? spawned->As<RE::Actor>() : nullptr;
    if (!actor) {
        SKSE::log::error("ProcgenNpc: PlaceObjectAtMe failed for minted base {:X}",
                         newBase->GetFormID());
        return nullptr;
    }

    // Nudge in front of the anchor (NpcGenerator forward-vector convention).
    const float angleZ = anchor->data.angle.z;
    RE::NiPoint3 pos = anchor->GetPosition();
    pos.x += std::sin(angleZ) * 150.0f;
    pos.y += std::cos(angleZ) * 150.0f;
    pos.z += 10.0f;
    actor->SetPosition(pos, true);
    actor->SetAngle(anchor->data.angle);

    // Optional ref-level scale (research §4 "體格"): applied to the ref, not the base.
    if (recipe.contains("scale") && recipe["scale"].is_number()) {
        const float scale = recipe["scale"].get<float>();
        if (scale > 0.0f) actor->SetScale(scale);
    }

    SKSE::log::info("ProcgenNpc: minted base {:X} -> ref {:X} (template {:X})",
                    newBase->GetFormID(), actor->GetFormID(), templateNpc->GetFormID());
    return actor;
}

// Capture the placed ref's position + cell/worldspace plugin FormIDs for the
// co-save (research §5 step 2: store position/cell, never the 0xFF ref id).
void CaptureLocation(TrackedNpc& t, RE::Actor* actor) {
    if (!actor) return;
    t.lastPos = actor->GetPosition();
    if (auto* cell = actor->GetParentCell()) {
        // Interior cells are plugin forms with stable FormIDs (ResolveFormID-safe).
        if (cell->GetFormID() < 0xFF000000) t.cellFormID = cell->GetFormID();
    }
    if (auto* world = actor->GetWorldspace()) {
        if (world->GetFormID() < 0xFF000000) t.worldFormID = world->GetFormID();
    }
}

// ---- co-save blob (de)serialization helpers ----
// Layout per record (research §5 step 2): we write the recipe as a length-
// prefixed JSON string plus the small fixed fields. We deliberately store the
// template *plugin* FormID separately (ResolveFormID remaps it on load) instead
// of trusting the copy embedded in the JSON, which is not auto-remapped.

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
        SKSE::log::error("ProcgenNpc: refusing absurd string length {}", len);
        return false;
    }
    out.resize(len);
    return intfc->ReadRecordData(out.data(), len) == len;
}

}  // namespace

// ---------------------------------------------------------------------------
// Generation core (research §1) — public entry from the adapter / demo spell.
// ---------------------------------------------------------------------------

std::string Generate(const nlohmann::json& recipe, RE::TESObjectREFR* anchor) {
    if (!recipe.is_object()) {
        SKSE::log::warn("ProcgenNpc: Generate called with a non-object recipe");
        return {};
    }

    RE::FormID templatePluginFormID = kDefaultTemplate;
    auto* templateNpc =
        ResolveTemplate(recipe.value("template", std::string{}), templatePluginFormID);

    auto* actor = MintAndPlace(templateNpc, recipe, anchor);
    if (!actor) return {};

    // Stable key: explicit "persist_key", else auto-generate a unique one
    // (research §1 / §5). The key is the logical identity carried across reloads.
    std::string key = recipe.value("persist_key", std::string{});
    if (key.empty()) {
        key = "gen_npc_" + std::to_string(AutoKeyCounter()++);
    }

    TrackedNpc t;
    t.key = key;
    t.templatePluginFormID = templatePluginFormID;
    t.recipeJson = recipe.dump();
    t.ref = RE::ObjectRefHandle(actor);  // Actor* -> TESObjectREFR handle (BSPointerHandle)
    CaptureLocation(t, actor);

    {
        std::scoped_lock lock(g_mutex);
        Registry()[key] = std::move(t);
    }
    SKSE::log::info("ProcgenNpc: tracked generated NPC key='{}' (template {:X})", key,
                    templatePluginFormID);
    return key;
}

// ---------------------------------------------------------------------------
// Co-save persistence (research §5 step 2/3 — the whole point).
// ---------------------------------------------------------------------------

void OnSave(SKSE::SerializationInterface* intfc) {
    std::scoped_lock lock(g_mutex);
    auto& reg = Registry();

    if (!intfc->OpenRecord(kRecordType, kRecordVersion)) {
        SKSE::log::error("ProcgenNpc: OnSave OpenRecord failed");
        return;
    }
    const auto count = static_cast<std::uint32_t>(reg.size());
    intfc->WriteRecordData(count);

    for (auto& [key, t] : reg) {
        // Refresh the saved position from the live ref if it's still valid, so a
        // reload restores it where the player last left it (research §5 step 2).
        if (auto refr = t.ref.get()) {
            t.lastPos = refr->GetPosition();
        }
        WriteString(intfc, key);
        // Store the EXISTING template plugin FormID — ResolveFormID remaps it on
        // load. NEVER the 0xFF dynamic base/ref id (research §5 step 2).
        intfc->WriteRecordData(t.templatePluginFormID);
        intfc->WriteRecordData(t.cellFormID);
        intfc->WriteRecordData(t.worldFormID);
        intfc->WriteRecordData(t.lastPos);
        WriteString(intfc, t.recipeJson);
    }
    SKSE::log::info("ProcgenNpc: OnSave wrote {} generated-NPC recipes", count);
}

void OnLoad(SKSE::SerializationInterface* intfc) {
    // OnLoad runs on the serialization thread; we MUST NOT mint/place here
    // (research §5 "時機"). Read the recipes, remap plugin FormIDs, and stage
    // them for RebuildStaged() on the next kPostLoadGame main-thread tick.
    g_staged.clear();

    std::uint32_t type = 0, version = 0, length = 0;
    while (intfc->GetNextRecordInfo(type, version, length)) {
        if (type != kRecordType) {
            SKSE::log::warn("ProcgenNpc: OnLoad skipping unknown record '{:08X}'", type);
            continue;
        }
        if (version != kRecordVersion) {
            SKSE::log::warn("ProcgenNpc: OnLoad record version {} != {} (ignored)", version,
                            kRecordVersion);
            continue;
        }
        std::uint32_t count = 0;
        if (intfc->ReadRecordData(count) == 0) {
            SKSE::log::error("ProcgenNpc: OnLoad failed to read count");
            break;
        }
        for (std::uint32_t i = 0; i < count; ++i) {
            StagedRecipe s;
            if (!ReadString(intfc, s.key)) break;
            RE::FormID storedTemplate = 0;
            intfc->ReadRecordData(storedTemplate);
            intfc->ReadRecordData(s.cellFormID);
            intfc->ReadRecordData(s.worldFormID);
            intfc->ReadRecordData(s.lastPos);
            if (!ReadString(intfc, s.recipeJson)) break;

            // Remap the EXISTING plugin FormIDs to this load order (research §5
            // step 3 / SerializationInterface::ResolveFormID).
            RE::FormID resolved = storedTemplate;
            if (!intfc->ResolveFormID(storedTemplate, resolved)) {
                SKSE::log::warn("ProcgenNpc: OnLoad ResolveFormID({:X}) failed; using as-is",
                                storedTemplate);
                resolved = storedTemplate;
            }
            s.templatePluginFormID = resolved;
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
    }
    SKSE::log::info("ProcgenNpc: OnLoad staged {} recipes for main-thread rebuild",
                    g_staged.size());
}

void OnRevert(SKSE::SerializationInterface*) {
    // Revert: drop the in-memory registry (research §5 "Revert"). The placed refs
    // belong to the outgoing save and are torn down by the engine.
    std::scoped_lock lock(g_mutex);
    Registry().clear();
    g_staged.clear();
    SKSE::log::info("ProcgenNpc: OnRevert cleared the in-memory registry");
}

bool Register(const SKSE::SerializationInterface* intfc) {
    if (!intfc) {
        SKSE::log::error("ProcgenNpc: Register got a null SerializationInterface");
        return false;
    }
    intfc->SetUniqueID(kSerializationUniqueID);
    intfc->SetSaveCallback(OnSave);
    intfc->SetLoadCallback(OnLoad);
    intfc->SetRevertCallback(OnRevert);
    SKSE::log::info("ProcgenNpc: registered co-save callbacks (uid '{:08X}')",
                    kSerializationUniqueID);
    return true;
}

// ---------------------------------------------------------------------------
// Rebuild-on-load (research §5 step 3) — runs on the main thread.
// ---------------------------------------------------------------------------

void RebuildStaged() {
    // Drain the staged recipes into a local copy so a re-entrant Generate during
    // rebuild doesn't fight the iterator. (Generate locks g_mutex; g_staged is
    // only touched on the main thread, so no extra lock needed for it here.)
    std::vector<StagedRecipe> staged;
    staged.swap(g_staged);
    if (staged.empty()) return;

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::error("ProcgenNpc: RebuildStaged has no player anchor");
        return;
    }

    {
        std::scoped_lock lock(g_mutex);
        Registry().clear();  // the old session's refs/bases are gone (research §5)
    }

    std::size_t rebuilt = 0;
    for (auto& s : staged) {
        nlohmann::json recipe;
        try {
            recipe = nlohmann::json::parse(s.recipeJson);
        } catch (const std::exception& e) {
            SKSE::log::error("ProcgenNpc: RebuildStaged bad recipe for '{}': {}", s.key, e.what());
            continue;
        }

        RE::FormID tplFormID = s.templatePluginFormID;
        auto* templateNpc = RE::TESForm::LookupByID<RE::TESNPC>(tplFormID);
        if (!templateNpc) {
            // Recipe's "template" string is the other source of truth (research
            // §5 step 3); fall back to resolving it if the remapped id missed.
            templateNpc = ResolveTemplate(recipe.value("template", std::string{}), tplFormID);
        }

        // Mint + place at the player (anchor). NOTE (research §5 caveat): this is
        // a FRESH base + ref — we cannot restore the old ref FormID. We re-place
        // at the saved world position so the NPC reappears at its last spot.
        auto* actor = MintAndPlace(templateNpc, recipe, player);
        if (!actor) {
            SKSE::log::warn("ProcgenNpc: RebuildStaged failed to mint '{}'", s.key);
            continue;
        }
        // Restore the saved position (MintAndPlace dropped it in front of the
        // player; move it to where it was saved). Cell/worldspace re-attachment
        // beyond a position set is left to the engine's normal cell streaming.
        if (s.lastPos.x != 0.0f || s.lastPos.y != 0.0f || s.lastPos.z != 0.0f) {
            actor->SetPosition(s.lastPos, true);
        }

        TrackedNpc t;
        t.key = s.key;
        t.templatePluginFormID = tplFormID;
        t.recipeJson = s.recipeJson;
        t.ref = RE::ObjectRefHandle(actor);  // Actor* -> TESObjectREFR handle
        t.cellFormID = s.cellFormID;
        t.worldFormID = s.worldFormID;
        t.lastPos = s.lastPos;
        {
            std::scoped_lock lock(g_mutex);
            Registry()[s.key] = std::move(t);
        }
        ++rebuilt;
        SKSE::log::info("ProcgenNpc: rebuilt '{}' -> ref {:X} at ({:.0f},{:.0f},{:.0f})", s.key,
                        actor->GetFormID(), s.lastPos.x, s.lastPos.y, s.lastPos.z);
    }
    SKSE::log::info("ProcgenNpc: RebuildStaged rebuilt {}/{} generated NPCs", rebuilt,
                    staged.size());
}

std::size_t TrackedCount() {
    std::scoped_lock lock(g_mutex);
    return Registry().size();
}

}  // namespace skyrim::procgen::npc
