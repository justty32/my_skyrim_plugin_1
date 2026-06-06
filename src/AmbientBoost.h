#pragma once

// AmbientBoost — brighten the WHOLE current interior cell by raising its INTERIOR_DATA ambient +
// directional + directional-ambient (DALC) values. This lifts shadowed areas across the entire cell
// (not just near a point light) and, being real lighting (albedo-modulated), keeps black armor dark.
//
// IMPORTANT engine limitation: the cell's lighting is only (re)applied by the engine on cell load /
// room transition — there is no public "refresh now". So the change takes effect after you EXIT and
// RE-ENTER the cell (same as Cell Patcher / Ambient Templates). A hotkey (default K) cycles levels;
// we save the cell's originals and restore them at level Off. Per-cell, no other cell affected.

namespace AmbientBoost
{
    void Initialize();   // register hotkey sink + load ini. Call once from kDataLoaded.

    // Restore every cell we modified back to its original lighting and clear state. Call on
    // kPreLoadGame so a loaded save never inherits this session's runtime ambient edits.
    void RestoreAll();
}
