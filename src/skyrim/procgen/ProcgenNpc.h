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

// Co-save RECORD id ('GNPC' = Generated NPC). This is a per-module record type
// under the plugin's ONE SerializationInterface unique id (which is owned by
// skyrim::cosave, NOT here — see CoSave.h on the one-SetUniqueID-per-plugin
// limit). We no longer call SetUniqueID/SetSaveCallback ourselves; we register a
// handler with the central dispatcher (research §5 / SKSE/Interfaces.h).
inline constexpr std::uint32_t kRecordType = 'GNPC';
// v2 adds the prior placed-ref FormID per record (so the load path can delete the
// engine-restored conjured actor before re-minting — fixes the save/load duplicate).
// A v1 save lacks that field; OnLoad ignores mismatched versions (the stale conjured
// actor in such a save can't be auto-stripped, but new saves are written as v2).
inline constexpr std::uint32_t kRecordVersion = 2;

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

// Co-save registration (research §5 step 2/3). Call ONCE from SKSEPluginLoad,
// BEFORE skyrim::cosave::Register. This no longer touches SetUniqueID / the
// callbacks (the central dispatcher owns the plugin's single registration —
// CoSave.h); it registers a 'GNPC' handler via cosave::AddHandler. Always
// succeeds (kept returning bool for call-site symmetry).
bool Register();

// Co-save handler callbacks (registered with the central dispatcher). Exposed for
// clarity/testing; the dispatcher invokes them on SKSE's serialization thread.
void OnSave(SKSE::SerializationInterface* intfc);  // write every tracked recipe
// OnLoad receives the record (version, length) already read by the dispatcher's
// GetNextRecordInfo, so it reads only this 'GNPC' record's payload (no header).
void OnLoad(SKSE::SerializationInterface* intfc, std::uint32_t version, std::uint32_t length);
void OnRevert(SKSE::SerializationInterface* intfc);  // clear the in-memory registry

// Rebuild every staged NPC from its recipe on the main thread. Call from the
// kPostLoadGame message handler (research §5 "時機": form system ready, after
// kDataLoaded). No-op if nothing was staged by OnLoad.
void RebuildStaged();

// In-memory tracked-NPC count (for logging / the demo).
std::size_t TrackedCount();

}  // namespace skyrim::procgen::npc
