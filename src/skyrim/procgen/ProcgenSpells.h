#pragma once

// ProcgenSpells (research/PROCGEN_INTERIOR.md §7.2, PROCGEN_EXTERIOR.md §8) —
// the DIRECT example spells layer. Mirrors NpcGenerator's proven recipe exactly:
// dynamically create a base EffectSetting + SpellItem (IFormFactory), name them
// "C++: ..." so the shared TESSpellCastEvent dispatch can fan them out, give them
// to the player on kNewGame / kPostLoadGame, and run the procgen on cast.
//
// Three spells (all lesser powers, fire-and-forget, self):
//   "C++: Generate Room"           -> Procgen::GenerateInterior (cottage recipe)
//   "C++: Conjure Keep"            -> Procgen::GenerateStructure (keep recipe)
//   "C++: Rearrange Furnishings"   -> Procgen::RearrangeFurnishings (last room)
//
// These spells load the SAME recipe JSON files the adapter actions use
// (config/procgen/recipe_cottage.json / recipe_keep.json), so the spell layer and
// the JSON layer are backed by one Procgen module.
//
// THREADING: the cast handler runs on the game's event thread for
// TESSpellCastEvent (same as NpcGenerator's handler) which is the main thread for
// these mutations; all RE:: pointers are null-checked.

namespace skyrim::procgen {

// Create the dynamic effect + spells and install the cast handler. Call once
// from kDataLoaded (after the game data is loaded), like NpcGenerator::InitializeMagic.
void InitializeSpells();

// Add the procgen spells to the player. Call on kNewGame and kPostLoadGame
// (dynamic forms are not persisted, so they must be re-added — MODDING_COOKBOOK §1.2).
void GiveSpellsToPlayer();

}  // namespace skyrim::procgen
