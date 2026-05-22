#pragma once

// Portable quest-engine core (SPEC §3 state machine, §4.1-4.3 core vocabulary,
// §4.5 async-message hooks). Loads one quest document and drives it via the
// capability ports. ZERO RE::/SKSE:: — only std + nlohmann-json.
//
// Phase 0 scope: core conditions/actions/triggers, synchronous dialogue flow,
// scheduling (schedule/timer), global vars, reset_quest. Persistence (§6),
// random/PRF (§8) and the Conditions/Actions/Triggers file split (DESIGN §6)
// are deferred to later phases.

#include <map>
#include <string>

#include <nlohmann/json.hpp>

#include "Ports.h"
#include "QuestState.h"

namespace qe {

class QuestEngine {
public:
    struct Deps {
        IDialoguePresenter* presenter = nullptr;
        IClock* clock = nullptr;
        ILogger* logger = nullptr;
        IConditionEvaluator* condEval = nullptr;  // adapter-extension conditions
        IActionRunner* actionRunner = nullptr;    // adapter-extension actions
        GlobalStore* globals = nullptr;           // SPEC §2.4
    };

    QuestEngine(nlohmann::json doc, Deps deps);

    // SPEC §3.1: apply initial state, run on_start, emit quest_start.
    void start();

    // Drive a (core or adapter) event into the trigger machine (SPEC §3.3).
    void dispatchEvent(const std::string& on,
                       const nlohmann::json& filter = nlohmann::json::object());

    // Fire any timers whose due time has passed (SPEC §4.2). Needs the clock.
    void checkTimers();

    bool terminated() const { return st_.terminated; }
    const QuestState& state() const { return st_; }

private:
    void applyInitialState();
    void runStart();  // on_start + quest_start (shared by start() and reset_quest)

    void fireTriggers(const std::string& on, const nlohmann::json& filter);
    bool triggerMatches(const nlohmann::json& trig, const std::string& on,
                        const nlohmann::json& filter) const;

    void runActions(const nlohmann::json& actions);
    void runAction(const nlohmann::json& action);
    bool evalCondition(const nlohmann::json& cond);

    void startDialogue(const std::string& id);

    Value getVar(const std::string& name);
    void setVar(const std::string& name, const Value& v);
    void addVar(const std::string& name, double delta);

    void log(const std::string& msg) const;

    nlohmann::json doc_;
    QuestState st_;
    Deps d_;
    std::map<std::string, double> timers_;  // key -> due game-hours (this quest)
};

}  // namespace qe
