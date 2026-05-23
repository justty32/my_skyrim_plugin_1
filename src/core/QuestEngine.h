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
#include <vector>

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

    // SPEC §7: offline validation pass. Walks the whole document against the
    // CORE vocabulary and structural rules, collecting every problem (it does
    // NOT stop at the first). Each message is prefixed with the quest id and the
    // JSON path of the offending node/choice/action/condition. Adapter-extension
    // verbs/conditions/events are NOT flagged (the core does not understand
    // them; §4.4 says they are checked by the merged "effective schema").
    // Returns the list of problems; empty == structurally valid for the core.
    // `start()` does NOT require this to have been run, but a host SHOULD call
    // it at load and refuse / warn on a non-empty result.
    std::vector<std::string> validate() const;

    // SPEC §3.1: apply initial state, run on_start, emit quest_start.
    void start();

    // Drive a (core or adapter) event into the trigger machine (SPEC §3.3).
    void dispatchEvent(const std::string& on,
                       const nlohmann::json& filter = nlohmann::json::object());

    // Fire any timers whose due time has passed (SPEC §4.2). Needs the clock.
    void checkTimers();

    // Resumable dialogue (SPEC §3.2): presentNode is display-only, so the
    // adapter feeds the player's pick here once the UI callback fires. idx is
    // 0-based into the choices last presented; idx<0 (or out of range) cancels.
    void submitChoice(int idx);
    // True while a choice node is presented and waiting for submitChoice().
    bool awaitingChoice() const { return awaitingChoice_; }

    bool terminated() const { return st_.terminated; }
    const QuestState& state() const { return st_; }

    // SPEC §6: serialize this quest's in-progress runtime state into a versioned,
    // self-describing JSON blob — quest vars, objective states, the current
    // dialogue node (if one is in progress), the terminated flag, and pending
    // timers (key -> ABSOLUTE due game-hours, §6/§8). Globals are NOT included
    // here: they live at the system level (§2.4) and are persisted separately via
    // the free serializeGlobals()/restoreGlobals() helpers below. Pure data — no
    // pointers, no game/runtime ids (§6 stable-string-id rule). Never throws.
    nlohmann::json exportProgress() const;

    // SPEC §6: restore a blob produced by exportProgress() onto this engine. The
    // host MUST have reloaded the quest DEFINITION (constructed the engine from
    // the same JSON) first; this layers the saved PROGRESS on top (§6 "定義與進度
    // 分離"). Fully tolerant: a missing/wrong-typed/extra field is ignored and
    // never throws, so a save written by an older/newer/partly-corrupt build
    // still loads as far as it can (degrade, don't crash). If a dialogue was in
    // progress it is re-presented so the presenter shows the saved node again and
    // awaitingChoice()/the pending-choice list are rebuilt (§6 resumable dialogue,
    // §4.5 unacked message still awaiting). Returns true if the blob was an object
    // it could read (even partially); false if it was not a usable object.
    bool importProgress(const nlohmann::json& blob);

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
    void presentCurrentNode();             // display st_.currentNode; await or end
    void endDialogue(const std::string& id);  // fire dialogue_end + clear state

    Value getVar(const std::string& name);
    void setVar(const std::string& name, const Value& v);
    void addVar(const std::string& name, double delta);

    void log(const std::string& msg) const;
    std::string questId() const;  // safe doc_ id read (tolerates a non-object doc)

    // One visible choice of the currently-awaited node, captured at present
    // time so submitChoice() is decoupled from re-evaluating doc_/conditions.
    struct PendingChoice {
        nlohmann::json then;   // actions to run on pick (null if none)
        std::string gotoNode;  // next node id ("" -> dialogue ends)
        bool end = false;      // explicit end after this pick
    };

    nlohmann::json doc_;
    QuestState st_;
    Deps d_;
    std::map<std::string, double> timers_;  // key -> due game-hours (this quest)
    bool awaitingChoice_ = false;
    std::vector<PendingChoice> pending_;     // choices for the awaited node
};

// SPEC §6: system-level globals (§2.4) are persisted SEPARATELY from any single
// quest's progress (reset_quest must not wipe them, and two quests share them).
// These free helpers (re)build a JSON object of {name -> scalar value} for the
// host to drop into the same co-save blob. restoreGlobals() is tolerant: a
// non-object, a missing key, or a non-scalar entry is skipped and never throws.
// To keep a global's declared TYPE stable across reload, restoreGlobals() only
// overwrites a value when the stored type matches an already-declared global's
// type (the host declares globals up front, §2.4/§7.6); unknown globals in the
// blob are still imported so a later-declared global isn't silently dropped.
nlohmann::json serializeGlobals(const GlobalStore& globals);
void restoreGlobals(GlobalStore& globals, const nlohmann::json& blob);

}  // namespace qe
