#pragma once

// =============================================================================
// Alchemy Spike  (research/ALCHEMY_SPIKE_FINDINGS.md)
//
// Riskiest-unknown spike for the 3D-physical-alchemy feature: prove that an
// SKSE plugin (CommonLibSSE-NG) can produce a *vanilla-correct* RE::AlchemyItem
// from N ingredients, register it so it persists in a save, and hand it to the
// player.
//
// Trigger: hotkey (default = F11, DX scancode 0x57) via an InputEvent sink.
// On press it brews a potion from two HARDCODED vanilla ingredients and logs
// the full result so numbers can be eyeballed against the vanilla AlchemyMenu.
//
// This file is self-contained; the only edits outside src/alchemy_spike/ are
// one Init() call in plugin.cpp (kDataLoaded) and the cmake list registration.
// =============================================================================

namespace AlchemySpike
{
    // Wire-up entry point. Call once from plugin.cpp on kDataLoaded.
    // Registers the hotkey input sink. Safe to call once.
    void Init();
}
