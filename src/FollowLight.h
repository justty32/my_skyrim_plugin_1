#pragma once

// FollowLight — a runtime fill light placed at the player to brighten shadowed areas WITHOUT
// washing out low-albedo things like black armor (a real light is modulated by albedo, unlike a
// screen post-process). A hotkey (default: L) cycles 4 brightness tiers; each places the bulb at
// the player's current spot (stationary "drop a sun" — re-press to relight elsewhere).
//
// Mechanism: the engine owns the light. We PlaceObjectAtMe a no-shadow / large-radius / warm LIGH
// "bulb" (4 tiers authored in ModForgeDaylight.esp, local FormIDs 0x800..0x803) and let the engine's
// cell/lighting pipeline render/cull/manage it. We do NOT touch ShadowSceneNode or the player skeleton
// (that races the engine's worker jobs and crashes — see PITFALLS.md #8). Requires ModForgeDaylight.esp.

namespace FollowLight
{
    // Register the hotkey + cell-attach sinks + load the ini. Call once from kDataLoaded.
    void Initialize();

    // Per-frame follow: reposition the bulb to the player. Call from the PlayerCharacter::Update
    // hook (hook.cpp) — runs on the main thread. Cheap no-op when the light is off / not following.
    void Update();
}
