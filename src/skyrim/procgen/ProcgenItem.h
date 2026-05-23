#pragma once

// ProcgenItem (research/PROCGEN_NPC_FORMS.md §0/§1/§5 — same form-mint + co-save
// rebuild pattern, applied to ITEM base forms; research/ALCHEMY_SPIKE_FINDINGS.md
// §2 — the dynamic-form persistence question for the WEAP/ARMO/MISC family) —
// runtime procedural ITEM generation at the FORM level, mirroring ProcgenNpc.
//
// WHAT THIS DOES: mint a brand-new TESObjectWEAP / TESObjectARMO / TESObjectMISC
// base form at runtime via IFormFactory::GetConcreteFormFactoryByType<T>()->Create()
// + a per-type deep field copy from a vanilla template, apply recipe overrides
// (name, gold value, weapon damage, armor rating, weight), and ADD it to the
// player's inventory (PlayerCharacter::AddObjectToContainer — the NpcGenerator /
// alchemy-spike convention). The minted base is a real item form with a dynamic
// 0xFF FormID, fully usable this session. Tracked in a registry by a stable key.
//
// WHY "Copy" IS NOT USED FOR WEAP/MISC (verified against vendored headers):
// TESObjectARMO OVERRIDES Copy (TESObjectARMO.h:61, vfunc 2F — it deep-copies the
// biped model + armorAddons + armorRating), so for armor we call base->Copy(t).
// TESObjectWEAP and TESObjectMISC do NOT override Copy; they inherit
// TESForm::Copy, annotated `// 2F - { return; }` (TESForm.h:170) — a no-op. So a
// `weap->Copy(template)` would leave the new weapon BLANK. Instead we deep-copy
// the relevant BaseFormComponent sub-objects via their CopyComponent (vfunc 03:
// TESModel/TESFullName/TESValueForm/TESWeightForm/BGSKeywordForm/TESEnchantableForm)
// plus a memberwise copy of the type-specific raw struct (TESObjectWEAP::weaponData
// / criticalData). This is the robust, header-verified path for all three types.
//
// PERSISTENCE DECISION (research §3.2 point 2 + ALCHEMY_SPIKE_FINDINGS §2): the
// vanilla save codec DOES serialize WEAP/ARMO/MISC dynamic forms (that is how
// player-enchanted items survive) — BUT community evidence (cited in §3.2) is
// that a reloaded dynamic form comes back with ONLY its form type: blank name, no
// model, zero weight/value. A natively-"persisted" generated item would therefore
// reload as junk. So we DO NOT rely on native persistence as the source of truth.
// Instead, exactly like ProcgenNpc, we register a 'GITM' co-save handler that
// stores {stableKey, template *plugin* FormID, recipe JSON} and REBUILDS each item
// on load (re-mint + re-add to player). The vanilla codec serializes the previous
// session's minted dynamic base into the player's inventory and restores it on
// load, so to keep the item from ACCUMULATING by `count` on every reload, the
// rebuild path (and a same-session re-Generate of the same key) first STRIPS the
// prior instance before re-adding the freshly minted one: it RemoveItem()s the
// same-session live base by pointer, and scans the player's inventory for a
// DYNAMIC (0xFF...) base of the same form type whose name matches the recipe's
// name and removes that too (best-effort — a nameless blank shell cannot be matched
// and is left alone; see ProcgenItem.cpp StripPriorInstances / RebuildStaged). Net
// result: exactly one instance per persist_key after rebuild. We NEVER persist the
// 0xFF dynamic FormID (research §5 step 2 / §3.1).
//
// THREADING: Generate() runs on the main thread (called from the SkyrimActions
// adapter path — already marshalled — and from the demo spell's TESSpellCastEvent
// sink — already main-thread). OnSave/OnLoad/OnRevert run on SKSE's serialization
// thread and only stage data; the actual re-mint is deferred to RebuildStaged()
// on the first kPostLoadGame tick (research §5 "時機" / MODDING_COOKBOOK R9). All
// RE:: pointers are null-checked.
//
// PLACEHOLDER TEMPLATES: real vanilla base FormIDs per type (Iron Sword / Iron
// Armor / Gold001) are cited in ProcgenItem.cpp; override per-recipe via "template".

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace SKSE {
class SerializationInterface;
}

namespace skyrim::procgen::item {

// Co-save RECORD id ('GITM' = Generated ITeM). A per-module record under the
// plugin's ONE SerializationInterface unique id (owned by skyrim::cosave — see
// CoSave.h). Distinct from 'GNPC'/'PRGN'/'QEST'.
inline constexpr std::uint32_t kRecordType = 'GITM';
inline constexpr std::uint32_t kRecordVersion = 1;

// Generate one item from a recipe and add it to the player's inventory.
//
// Recipe shape:
//   {
//     "type": "weapon" | "armor" | "misc",   // required; selects the form type
//     "template": "0x...~Skyrim.esm" | "0x..." | "<EditorID>",  // base item
//     "name": "...",                         // optional SetFullName override
//     "persist_key": "...",                  // optional stable id (auto if absent)
//     "count": 1,                            // optional inventory count
//     "value": 250,                          // optional gold value override
//     "weight": 8.0,                         // optional weight override
//     "damage": 12,                          // optional (weapon) attack damage
//     "armor": 30                            // optional (armor) armor rating (CK points)
//   }
//
// Returns the stable key under which the item was tracked, or "" on failure.
std::string Generate(const nlohmann::json& recipe);

// Co-save registration. Call ONCE from SKSEPluginLoad, BEFORE cosave::Register.
// Registers a 'GITM' handler via cosave::AddHandler (does NOT touch SetUniqueID).
bool Register();

// Co-save handler callbacks (registered with the central dispatcher).
void OnSave(SKSE::SerializationInterface* intfc);
void OnLoad(SKSE::SerializationInterface* intfc, std::uint32_t version, std::uint32_t length);
void OnRevert(SKSE::SerializationInterface* intfc);

// Rebuild every staged item from its recipe on the main thread. Call from the
// kPostLoadGame message handler (after OnLoad staged the recipes). No-op if empty.
void RebuildStaged();

// In-memory tracked-item count (for logging / the demo).
std::size_t TrackedCount();

}  // namespace skyrim::procgen::item
