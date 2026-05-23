#pragma once

// SkyrimAdapter (DESIGN §6) — binds the portable quest-engine core to Skyrim.
// Owns a qe::QuestEngine, a qe::GlobalStore, and the six capability ports, builds
// QuestEngine::Deps, loads a quest JSON from the plugin config dir, and starts
// the engine. The MessageBox callback routes the player's pick back here via
// SubmitChoice(); game events route in via the EventSource sink.
//
// Phase 1 scope (per task): MessageBox dialogue, RE::Calendar clock, SKSE::log
// logger, the safe Actions/Conditions, EntityResolver (existing + spawn), and a
// scaffolded EventSource. Co-save persistence (SPEC §6) is now wired via a 'QEST'
// record on the central dispatcher (see Register/OnSave/OnLoad/OnRevert/
// RebuildStaged below). DEFERRED: native dialogue, random/PRF, the real
// cw_whiterun_*.json quests.
//
// THREADING: every public mutator (Start/SubmitChoice/FireEvent/CheckTimers)
// runs the engine on the main thread (SKSE::GetTaskInterface()->AddTask) so all
// RE:: side effects inside the ports are main-thread.

#include <cstdint>
#include <memory>
#include <string>

#include "core/QuestEngine.h"
#include "core/QuestState.h"
#include "SkyrimActions.h"
#include "SkyrimConditions.h"
#include "SkyrimEntities.h"
#include "SkyrimEvents.h"
#include "dialogue/MessageBoxPresenter.h"

namespace SKSE {
class SerializationInterface;
}

namespace skyrim {

class SkyrimAdapter {
public:
    static SkyrimAdapter* GetSingleton();

    // Load the demo quest JSON from the config dir and BUILD the engine (no
    // start() — on_start side effects belong to a fresh new game or to restored
    // progress, not to main-menu/kDataLoaded time). Safe to call from kDataLoaded.
    // Returns false if the quest file could not be loaded/parsed.
    bool LoadDemoQuest();

    // Run the engine's on_start (intro message + initial schedule) for a brand
    // new game. Call from kNewGame; marshalled onto the main thread. On a save
    // load this is skipped — RebuildStaged() restores saved progress instead, so
    // the intro message does not re-fire spuriously (see M2).
    void StartNewQuest();

    // Resume a dialogue after the player picks (from the MessageBox callback).
    void SubmitChoice(int idx);

    // Inject a game (or manual/debug) event into the engine.
    void FireEvent(const std::string& on, const nlohmann::json& filter);

    // Advance timers (call when game time may have moved; e.g. on a periodic
    // poll or after a wait/sleep). Phase 1 leaves the polling cadence to the
    // caller — see the test plan in the final report.
    void CheckTimers();

    SkyrimEvents& Events() { return events_; }

    // ---- co-save persistence (SPEC §6) ----------------------------------------
    // Co-save RECORD id ('QEST' = quest-engine progress). This is a per-module
    // record type under the plugin's ONE SerializationInterface unique id (owned
    // by skyrim::cosave, NOT here — CoSave.h on the one-SetUniqueID-per-plugin
    // limit). We register a handler with the central dispatcher, never call
    // SetUniqueID/SetSaveCallback ourselves. Mirrors ProcgenNpc's 'GNPC' handler.
    static constexpr std::uint32_t kRecordType = 'QEST';
    static constexpr std::uint32_t kRecordVersion = 1;

    // Register the 'QEST' handler with the central dispatcher. Call ONCE from
    // SKSEPluginLoad, BEFORE skyrim::cosave::Register (next to npc::Register()).
    // Does not touch SetUniqueID/the callbacks. Always succeeds (bool for symmetry).
    static bool Register();

    // Co-save handler callbacks (registered with the central dispatcher; the
    // dispatcher invokes them on SKSE's serialization thread — NOT the main
    // thread, so they only (de)serialize the pure-JSON state blob and STAGE it;
    // no RE:: live state is touched here — see RebuildStaged).
    static void OnSave(SKSE::SerializationInterface* intfc);
    // OnLoad receives the record (version, length) the dispatcher already read via
    // GetNextRecordInfo, so it reads only this 'QEST' record's payload.
    static void OnLoad(SKSE::SerializationInterface* intfc, std::uint32_t version,
                       std::uint32_t length);
    static void OnRevert(SKSE::SerializationInterface* intfc);

    // Apply the staged blob (progress + globals) onto the live engine on the MAIN
    // thread. Call from kPostLoadGame (after LoadDemoQuest built the engine at
    // kDataLoaded). No-op if nothing was staged by OnLoad (e.g. a brand new game,
    // which runs on_start via StartNewQuest at kNewGame instead).
    static void RebuildStaged();

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
