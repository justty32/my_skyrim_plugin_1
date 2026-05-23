#pragma once

// SkyrimAdapter (DESIGN §6) — binds the portable quest-engine core to Skyrim.
// Owns a qe::QuestEngine, a qe::GlobalStore, and the six capability ports, builds
// QuestEngine::Deps, loads a quest JSON from the plugin config dir, and starts
// the engine. The MessageBox callback routes the player's pick back here via
// SubmitChoice(); game events route in via the EventSource sink.
//
// Phase 1 scope (per task): MessageBox dialogue, RE::Calendar clock, SKSE::log
// logger, the safe Actions/Conditions, EntityResolver (existing + spawn), and a
// scaffolded EventSource. DEFERRED: co-save persistence (SPEC §6), native
// dialogue, random/PRF, the real cw_whiterun_*.json quests.
//
// THREADING: every public mutator (Start/SubmitChoice/FireEvent/CheckTimers)
// runs the engine on the main thread (SKSE::GetTaskInterface()->AddTask) so all
// RE:: side effects inside the ports are main-thread.

#include <memory>
#include <string>

#include "core/QuestEngine.h"
#include "core/QuestState.h"
#include "SkyrimActions.h"
#include "SkyrimConditions.h"
#include "SkyrimEntities.h"
#include "SkyrimEvents.h"
#include "dialogue/MessageBoxPresenter.h"

namespace skyrim {

class SkyrimAdapter {
public:
    static SkyrimAdapter* GetSingleton();

    // Load the demo quest JSON from the config dir and start the engine. Safe to
    // call from kDataLoaded; the heavy work is marshalled onto the main thread.
    // Returns false if the quest file could not be loaded/parsed.
    bool StartDemoQuest();

    // Resume a dialogue after the player picks (from the MessageBox callback).
    void SubmitChoice(int idx);

    // Inject a game (or manual/debug) event into the engine.
    void FireEvent(const std::string& on, const nlohmann::json& filter);

    // Advance timers (call when game time may have moved; e.g. on a periodic
    // poll or after a wait/sleep). Phase 1 leaves the polling cadence to the
    // caller — see the test plan in the final report.
    void CheckTimers();

    SkyrimEvents& Events() { return events_; }

private:
    SkyrimAdapter() = default;

    // Locate the quest file: SKSE/Plugins/<CONFIG_FOLDER>/quests/<name>, with a
    // fall back to a Data-relative path if the config dir isn't found.
    std::string ResolveQuestPath(const std::string& fileName) const;

    // Build Deps and construct the engine from an already-parsed document.
    bool BuildEngine(nlohmann::json doc);

    qe::GlobalStore globals_;
    SkyrimEntities entities_;
    MessageBoxPresenter presenter_;
    SkyrimActions actions_{entities_};
    SkyrimConditions conditions_{entities_};
    SkyrimEvents events_;

    // Clock/Logger are tiny; defined in the .cpp and held by unique_ptr so the
    // header stays free of their RE::/SKSE:: bodies.
    std::unique_ptr<qe::IClock> clock_;
    std::unique_ptr<qe::ILogger> logger_;

    std::unique_ptr<qe::QuestEngine> engine_;
};

}  // namespace skyrim
