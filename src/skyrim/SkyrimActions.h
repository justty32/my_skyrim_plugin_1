#pragma once

// SkyrimActions (DESIGN §2.2) — ActionRunner port (qe::IActionRunner, SPEC §5.2).
// Executes adapter-extension action verbs as game side effects.
//
// THREADING: the engine invokes run() synchronously from start()/dispatchEvent()/
// submitChoice()/checkTimers(); the adapter guarantees all of those run on the
// main thread (SKSE::GetTaskInterface()->AddTask), so these RE:: mutations are
// already main-thread. Every pointer is null-checked (MODDING_COOKBOOK).
//
// Implemented (low-risk, existing patterns): give_gold / remove_gold,
// give_item / remove_item, add_spell / remove_spell, spawn_character,
// move_character, deliver_letter (minimal: note in inventory + notification).
// High-risk / not-yet verbs (start_combat, add_shout, teleport_player,
// set_relationship, play_idle, add_map_marker, play_sound) log a clear
// "TODO not implemented" and no-op.

#include "core/Ports.h"
#include "SkyrimEntities.h"

namespace skyrim {

class SkyrimActions : public qe::IActionRunner {
public:
    explicit SkyrimActions(SkyrimEntities& entities) : entities_(entities) {}

    void run(const std::string& verb, const nlohmann::json& params) override;

private:
    SkyrimEntities& entities_;
};

}  // namespace skyrim
