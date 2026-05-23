#pragma once

// ProcgenNpc (research/PROCGEN_NPC_FORMS.md §1, §5, §11) — runtime procedural
// NPC generation at the FORM level, with co-save "logical" persistence.
//
// WHAT THIS DOES (research §0/§1): mint a brand-new TESNPC_ base form at runtime
// via IFormFactory + Copy(template) (the proven NpcGenerator::SpawnNpc /
// SkyrimEntities::spawnCharacter recipe), apply recipe overrides (name, scale,
// essential, level, sex), and place a reference near an anchor. The minted base
// is a real TESNPC form with a dynamic 0xFF FormID — fully usable this session.
//
// WHY IT NEEDS A REBUILD-ON-LOAD CO-SAVE (research §3, §8, §10 — the hard limit):
// a runtime-minted TESNPC does NOT survive save/reload. The save codec does not
// serialize FormType::NPC; dynamic 0xFF FormIDs are a session-local counter
// (TESDataHandler::nextID) and are not stable across reloads. SKSE's
// SerializationInterface::ResolveFormID can only remap EXISTING plugin FormIDs —
// it cannot revive a base form we invented. So the only durable thing is a
// RECIPE: store {stable string key, template *plugin* FormID, recipe JSON,
// position/cell} in the .skse co-save and REBUILD each NPC on load. We NEVER
// store the 0xFF dynamic FormID or any raw pointer (research §5 step 2).
//
// HONEST CAVEAT (research §5 "對「持久身分」的誠實話"): this is *logical*
// persistence keyed by the stable string id, NOT engine-level stable-FormID
// identity. The original placed ref + minted base are gone after a reload (the
// engine never saved them); the load path mints a FRESH base + ref. Anything
// that referenced the old ref FormID, quest aliases pointing at the old base,
// and TESNPC::relationships all break and would need re-linking from the recipe.
// For "the same logical NPC reappears at its spot" (the court-wizard victim use
// case) this is sufficient; for "another mod permanently references it by
// FormID" it is not (that requires an ESP/ESL — research §6/§8).
//
// THREADING: every entry point touches RE:: state and MUST run on the main
// thread. Generate() is called from the SkyrimActions adapter path (already
// marshalled via SKSE::GetTaskInterface()->AddTask in SkyrimAdapter) and from
// the demo spell's TESSpellCastEvent sink (already main-thread). The co-save
// LOAD callback runs on SKSE's serialization thread, so RebuildAll() does NOT
// touch RE:: directly — it stages the recipes and defers the actual minting to
// the first kPostLoadGame tick via SKSE::GetTaskInterface()->AddTask (research
// §5 "時機" / MODDING_COOKBOOK R9). All RE:: pointers are null-checked.
//
// PLACEHOLDER: the default template FormID is a real vanilla generic NPC; see
// kDefaultTemplate in ProcgenNpc.cpp and the TODO there. Override per-recipe.

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace SKSE {
class SerializationInterface;
}

namespace skyrim::procgen::npc {

// Co-save record id ('GNPC' = Generated NPC). Unique within this plugin's
// SerializationInterface namespace (research §5 / SKSE/Interfaces.h SetUniqueID).
inline constexpr std::uint32_t kSerializationUniqueID = 'GNPC';
inline constexpr std::uint32_t kRecordType = 'GNPC';
inline constexpr std::uint32_t kRecordVersion = 1;

// Generate one NPC from a recipe and place a ref near the anchor (research §1).
//
// Recipe shape (research §11):
//   {
//     "template": "0x...~Skyrim.esm" | "0x..." | "<EditorID>",  // base TESNPC
//     "name": "...",                  // optional SetFullName override
//     "persist_key": "...",           // optional stable id (auto-generated if absent)
//     "scale": 1.0,                   // optional ref scale
//     "essential": true,              // optional kEssential base flag
//     "level": 10,                    // optional base level
//     "sex": "female"|"male"          // optional kFemale base flag
//   }
//
// Returns the stable key under which the NPC was tracked, or "" on failure.
// `anchor` is where the ref is placed (player / a marker ref). The recipe JSON
// is stored verbatim in the registry so the load path can rebuild identically.
std::string Generate(const nlohmann::json& recipe, RE::TESObjectREFR* anchor);

// Co-save registration (research §5 step 2/3). Call ONCE from SKSEPluginLoad
// after SKSE::Init, using SKSE::GetSerializationInterface(). Sets the unique id
// and the save/load/revert callbacks. Returns false if the interface is null.
bool Register(const SKSE::SerializationInterface* intfc);

// SerializationInterface callbacks (registered by Register()). Exposed for
// clarity/testing; the engine invokes them on its serialization thread.
void OnSave(SKSE::SerializationInterface* intfc);    // write every tracked recipe
void OnLoad(SKSE::SerializationInterface* intfc);    // read recipes, stage rebuild
void OnRevert(SKSE::SerializationInterface* intfc);  // clear the in-memory registry

// Rebuild every staged NPC from its recipe on the main thread. Call from the
// kPostLoadGame message handler (research §5 "時機": form system ready, after
// kDataLoaded). No-op if nothing was staged by OnLoad.
void RebuildStaged();

// In-memory tracked-NPC count (for logging / the demo).
std::size_t TrackedCount();

}  // namespace skyrim::procgen::npc
