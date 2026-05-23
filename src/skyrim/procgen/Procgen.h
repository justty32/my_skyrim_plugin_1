#pragma once

// Procgen (research/PROCGEN_INTERIOR.md §7-§8, research/PROCGEN_EXTERIOR.md §8-§9)
// — runtime procedural placement of vanilla base objects, built on the proven
// NpcGenerator primitives (PlaceObjectAtMe / CreateReferenceAtLocation, EditorID
// lookup, forward-vector anchoring). NO Creation Kit / ESP: every piece is an
// existing TESBoundObject placed at a computed transform.
//
// Two layers share this one module:
//   A. Direct example spells (ProcgenSpells.{h,cpp}) — "C++: Generate Room",
//      "C++: Conjure Keep", "C++: Rearrange Furnishings".
//   B. JSON-driven adapter actions (SkyrimActions verbs generate_interior /
//      generate_structure / rearrange_furnishings) parse the same Recipe shape.
//
// THREADING: every entry point here touches RE:: state and MUST run on the main
// thread. The spell path is already on the main thread (TESSpellCastEvent sink);
// the adapter path is marshalled via SKSE::GetTaskInterface()->AddTask before
// SkyrimActions::run is reached (SkyrimAdapter::OnMainThread). All RE:: pointers
// are null-checked (MODDING_COOKBOOK §1.1).
//
// PERSISTENCE (research §5 / §5.2 strategy B — NOW IMPLEMENTED): generated refs
// are placed forcePersist=false; the dynamic 0xFF ref FormIDs are NEVER stored.
// Instead each generated room/structure records {stable key, kind, the full
// self-contained recipe JSON, the ABSOLUTE world origin + yaw + seed actually
// used, and the cell/worldspace *plugin* FormID (ResolveFormID-remappable)} into
// the SKSE co-save via the central skyrim::cosave dispatcher (record type
// 'PRGN'). On load these are staged, plugin FormIDs are remapped, and
// RebuildStaged() (kPostLoadGame, main thread) re-runs GenerateInterior/
// GenerateStructure at the stored origin so the same building reappears in place.
//
// HONEST CAVEAT — this is LOGICAL persistence, keyed by the stable string id, not
// engine-level stable-FormID identity. The rebuilt refs are FRESH (new 0xFF ids);
// the original placed refs are gone after a reload (the engine never saved them).
// This is fine for "the building reappears at its spot," but NOT for anything
// that references those refs by FormID (quest aliases, other mods, links). That
// would require an ESP/ESL (research §5 / PROCGEN_NPC_FORMS §6/§8).
//
// PLACEHOLDERS: EditorIDs for kit pieces / furniture are real vanilla strings
// where known, but several need in-game validation (see kPlaceholder notes in
// Procgen.cpp and the recipe JSON comments). Unknown ones fall back gracefully
// and log a warning rather than crashing.

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace SKSE {
class SerializationInterface;
}

namespace skyrim::procgen {

// One placed piece description, resolved from a recipe entry. Offsets are in the
// recipe's LOCAL frame (relative to the structure origin, before the anchor yaw
// is applied); rotations are stored in DEGREES as authored and converted to
// radians at placement time (MathUtil::Angle::DegreeToRadian).
struct PieceSpec {
    std::string base;            // EditorID or "0x..~Mod.esp" (FormUtil::Parse)
    float localX = 0.f;          // local-frame offset (units)
    float localY = 0.f;
    float localZ = 0.f;
    float rotXDeg = 0.f;         // local euler rotation (degrees)
    float rotYDeg = 0.f;
    float rotZDeg = 0.f;
    std::string motion = "fixed";  // "fixed" | "keyframed" | "dynamic"
    bool anchorGround = false;   // snap this piece's Z to land height (exterior)
    std::string slot;            // furniture slot tag (for rearrange tracking)
};

// A parsed recipe. Covers both the interior (generate_interior) and exterior
// (generate_structure) shapes; fields that don't apply to one stay at defaults.
struct Recipe {
    std::string templateId;
    float gridStep = 256.f;      // interior kit grid pitch (units)
    float anchorDistance = 300.f; // how far in front of the caster the origin sits
    std::uint32_t seed = 1337;   // deterministic clutter scatter seed
    std::string groundRule = "none";  // "none" | "per_piece" | "flatten_to_max"
    float cellSize = 1024.f;     // exterior footprint cell size (for flatten sampling)
    int footprintCellsX = 3;
    int footprintCellsY = 3;

    std::vector<PieceSpec> shell;       // walls/floors/roof/towers/gates
    std::vector<PieceSpec> furniture;   // furniture slots (interior)
    std::vector<PieceSpec> lights;      // light refs (interior)
    std::vector<PieceSpec> clutter;     // scattered misc clutter (interior)
};

// Parse a recipe document (the JSON shapes from the research docs §8/§9). Robust
// to missing fields; returns false + logs only on a fundamentally unusable doc.
bool ParseRecipe(const nlohmann::json& doc, Recipe& out);

// Load a recipe JSON file from the config dir (config/procgen/<file>). Returns
// false if the file is missing / unparseable.
bool LoadRecipeFile(const std::string& fileName, Recipe& out);

// Generate an interior-style room from a recipe at the anchor (in front of the
// caster, anchorDistance units). Tracks all placed refs under `persistKey` so
// they can be cleared/rearranged. Returns the number of refs placed.
//
// `clutterDoc` is the optional raw "clutter" JSON array (count/seed/scatter_aabb
// per entry); when supplied (the JSON-driven adapter path) clutter is scattered
// deterministically inside its AABB. The spell path passes an empty json and
// clutter falls back to its authored single positions.
int GenerateInterior(const Recipe& recipe, RE::TESObjectREFR* anchor,
                     const std::string& persistKey,
                     const nlohmann::json& clutterDoc = nlohmann::json::array());

// Generate an exterior modular structure (keep) from a recipe at the anchor.
// Ground-anchors per ground_rule (RE::TES::GetLandHeight). Returns refs placed.
int GenerateStructure(const Recipe& recipe, RE::TESObjectREFR* anchor,
                      const std::string& persistKey);

// Re-place / swap the furniture of a previously generated room. Without a key,
// rearranges the most-recently generated room. Returns the number of furniture
// refs re-placed (0 if nothing tracked). Deterministic per call-count so a
// repeated cast cycles through a few layouts.
int RearrangeFurnishings(const std::string& persistKey = "");

// Disable + mark-for-delete every tracked ref under a key (or all keys if empty)
// and drop them from the registry, so generated content does not leak. Also drops
// the corresponding co-save record(s) so a cleared room does not reappear on the
// next reload (research §5).
void ClearGenerated(const std::string& persistKey = "");

// Serialize a parsed Recipe back to a SELF-CONTAINED JSON doc (independent of the
// original recipe file, which may not exist at load time). This is what the
// co-save stores so RebuildStaged() can re-parse + rebuild deterministically.
// Round-trips through ParseRecipe (the resulting doc parses back to an equivalent
// Recipe, including clutter scatter parameters).
nlohmann::json RecipeToJson(const Recipe& recipe);

// ---- Co-save persistence (research §5.2 strategy B). Registered as record type
// 'PRGN' with the central skyrim::cosave dispatcher (CoSave.h) — procgen does NOT
// own a SetUniqueID of its own. ---------------------------------------------------

inline constexpr std::uint32_t kRecordType = 'PRGN';  // Procgen generated content
inline constexpr std::uint32_t kRecordVersion = 1;

// Register procgen's 'PRGN' handler with the central dispatcher. Call ONCE from
// SKSEPluginLoad, before skyrim::cosave::Register. Always succeeds.
bool Register();

// Co-save handler callbacks (invoked by the dispatcher on the serialization
// thread). Exposed for clarity/testing.
void OnSave(SKSE::SerializationInterface* intfc);  // write every tracked room/structure
// OnLoad receives the record (version, length) already read by the dispatcher;
// it reads only this 'PRGN' record's payload (no header).
void OnLoad(SKSE::SerializationInterface* intfc, std::uint32_t version, std::uint32_t length);
void OnRevert(SKSE::SerializationInterface* intfc);  // clear the in-memory registry

// Rebuild every staged room/structure from its recipe at the stored absolute
// origin + seed, on the main thread. Call from kPostLoadGame (research §5 "時機":
// form system ready, after OnLoad staged the records). No-op if nothing staged.
void RebuildStaged();

}  // namespace skyrim::procgen
