#include "QuestEngine.h"

#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace qe {

namespace {
constexpr const char* kGlobalPrefix = "global.";
constexpr std::size_t kGlobalPrefixLen = 7;

Value jsonToValue(const nlohmann::json& j) {
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number()) return j.get<double>();
    if (j.is_string()) return j.get<std::string>();
    return std::string{};  // tolerant fallback
}

// SPEC §6 (de)serialization: a Value -> a JSON scalar of the SAME type, so a
// round-trip (export -> import) preserves bool vs number vs string (variant
// equality and var_eq are type-aware, §4.1). The inverse is jsonToValue above.
nlohmann::json valueToJson(const Value& v) {
    return std::visit([](auto&& x) -> nlohmann::json { return x; }, v);
}

bool isGlobal(const std::string& name) {
    return name.rfind(kGlobalPrefix, 0) == 0;
}

// Safe accessors: nlohmann's operator[] / get<T>() throw (or assert) on a
// missing key or a type mismatch, which would crash the host on malformed
// content. SPEC §8 says a problem reached at runtime MUST be logged and the
// current flow aborted safely, never crash. These helpers return a default and
// let the caller decide. (Structural problems SHOULD be caught earlier by
// validate(), but the runtime stays defensive regardless.)

const nlohmann::json* jsonAt(const nlohmann::json& j, const char* key) {
    if (!j.is_object()) return nullptr;
    auto it = j.find(key);
    return it != j.end() ? &(*it) : nullptr;
}

// A scalar value (number/bool/string) at j[key], or nullopt if absent / not a
// scalar. Used by set_var/var_eq etc. where the parameter is a JSON value.
bool tryGetString(const nlohmann::json& j, const char* key, std::string& out) {
    const nlohmann::json* p = jsonAt(j, key);
    if (!p || !p->is_string()) return false;
    out = p->get<std::string>();
    return true;
}

bool tryGetNumber(const nlohmann::json& j, const char* key, double& out) {
    const nlohmann::json* p = jsonAt(j, key);
    if (!p || !p->is_number()) return false;
    out = p->get<double>();
    return true;
}

// An id-shorthand parameter: SPEC §4.2 allows {"complete_objective":"obj_id"}.
// Accepts a bare string. Returns false (and leaves out untouched) otherwise.
bool tryGetIdString(const nlohmann::json& p, std::string& out) {
    if (!p.is_string()) return false;
    out = p.get<std::string>();
    return true;
}
}  // namespace

QuestEngine::QuestEngine(nlohmann::json doc, Deps deps)
    : doc_(std::move(doc)), d_(deps) {}

void QuestEngine::log(const std::string& msg) const {
    if (d_.logger) d_.logger->log(msg);
}

// json::value() throws on a non-object; this stays safe so log lines never
// crash even when the whole document is malformed (e.g. a top-level array).
std::string QuestEngine::questId() const {
    return doc_.is_object() ? doc_.value("id", std::string{"?"}) : std::string{"?"};
}

// ---- variable access (SPEC §2.4: global.<name> resolves to the GlobalStore) ----

Value QuestEngine::getVar(const std::string& name) {
    if (isGlobal(name)) {
        const std::string g = name.substr(kGlobalPrefixLen);
        if (d_.globals) {
            auto it = d_.globals->vars.find(g);
            if (it != d_.globals->vars.end()) return it->second;
        }
        log("global var not declared: " + name + " (treating as 0)");
        return 0.0;
    }
    auto it = st_.vars.find(name);
    if (it != st_.vars.end()) return it->second;
    log("var not found: " + name + " (treating as 0)");
    return 0.0;
}

void QuestEngine::setVar(const std::string& name, const Value& v) {
    if (isGlobal(name)) {
        const std::string g = name.substr(kGlobalPrefixLen);
        if (d_.globals) {
            d_.globals->vars[g] = v;
        } else {
            log("set global '" + name + "' but no GlobalStore");
        }
        return;
    }
    st_.vars[name] = v;
}

void QuestEngine::addVar(const std::string& name, double delta) {
    setVar(name, Value{asDouble(getVar(name)) + delta});
}

// ---- lifecycle (SPEC §3.1) ----

void QuestEngine::applyInitialState() {
    st_ = QuestState{};
    timers_.clear();  // reset clears this quest's pending timers
    awaitingChoice_ = false;  // and any dialogue in progress
    pending_.clear();
    started_ = false;  // un-started until runStart() runs on_start (set there)
    if (const nlohmann::json* vars = jsonAt(doc_, "vars"); vars && vars->is_object()) {
        for (auto& [k, v] : vars->items()) st_.vars[k] = jsonToValue(v);
    }
    if (const nlohmann::json* objs = jsonAt(doc_, "objectives"); objs && objs->is_object()) {
        for (auto& [k, o] : objs->items())
            st_.objectives[k] = o.is_object() ? o.value("state", "inactive") : "inactive";
    }
}

void QuestEngine::runStart() {
    if (doc_.contains("on_start")) runActions(doc_["on_start"]);
    fireTriggers("quest_start", nlohmann::json::object());
    // on_start has now actually executed. Set the persisted started_ flag LAST so
    // both start() and reset_quest (applyInitialState()+runStart()) end up started.
    // resetState() calls applyInitialState() WITHOUT runStart(), so it stays false.
    started_ = true;
}

void QuestEngine::start() {
    applyInitialState();
    runStart();
}

void QuestEngine::resetState() {
    // Side-effect-free reset: the same state wipe start() does, WITHOUT runStart()
    // (no on_start, no quest_start). For hosts resetting off the main thread.
    applyInitialState();
}

// ---- triggers (SPEC §3.3) ----

void QuestEngine::dispatchEvent(const std::string& on, const nlohmann::json& filter) {
    fireTriggers(on, filter);
}

bool QuestEngine::triggerMatches(const nlohmann::json& trig, const std::string& on,
                                 const nlohmann::json& filter) const {
    if (!trig.is_object()) return false;
    if (trig.value("on", std::string{}) != on) return false;
    // SPEC §3.3: every filter key on the trigger must equal the event's value
    // for that key. The structural keys on/when/do are NOT filters. Treating
    // any other key generically (rather than a hardcoded list) keeps the core
    // adapter-agnostic: a Skyrim event filtered by "form" and a Godot event
    // filtered by "node" both work without the core knowing those names.
    for (auto& [k, v] : trig.items()) {
        if (k == "on" || k == "when" || k == "do") continue;
        if (!filter.contains(k) || filter[k] != v) return false;
    }
    return true;
}

void QuestEngine::fireTriggers(const std::string& on, const nlohmann::json& filter) {
    // SPEC §3.1.3: a terminated quest MUST stop responding to its triggers.
    // (reset_quest clears st_.terminated before re-running, so loops still work.)
    if (st_.terminated) return;
    const nlohmann::json* trigs = jsonAt(doc_, "triggers");
    if (!trigs) return;
    if (!trigs->is_array()) {
        log(questId() + ": 'triggers' is not an array");
        return;
    }
    for (auto& trig : *trigs) {
        if (!trig.is_object()) continue;  // validate() reports; stay safe at runtime
        if (!triggerMatches(trig, on, filter)) continue;
        if (trig.contains("when") && !evalCondition(trig["when"])) continue;
        if (trig.contains("do")) runActions(trig["do"]);
        if (st_.terminated) break;
    }
}

void QuestEngine::checkTimers() {
    if (st_.terminated) return;  // SPEC §3.1.3: terminated quest stops responding
    if (!d_.clock) {
        if (!timers_.empty()) log("checkTimers called but no Clock port");
        return;
    }
    const double now = d_.clock->gameHours();
    std::vector<std::string> due;
    for (auto& [k, t] : timers_)
        if (t <= now) due.push_back(k);
    for (auto& k : due) {
        timers_.erase(k);
        nlohmann::json filter;
        filter["key"] = k;
        fireTriggers("timer", filter);
        if (st_.terminated) break;
    }
}

void QuestEngine::debugFireAllTimers() {
    if (st_.terminated) return;  // SPEC §3.1.3: terminated quest stops responding
    // Snapshot the keys first: fireTriggers("timer", ...) may re-schedule (a "do"
    // could schedule a new timer) or reset_quest (which clears timers_), so we
    // must not iterate timers_ while mutating it. Same per-key logic as
    // checkTimers(), only WITHOUT the due-time gate (every pending timer fires).
    std::vector<std::string> keys;
    keys.reserve(timers_.size());
    for (auto& [k, t] : timers_) keys.push_back(k);
    log(questId() + ": debugFireAllTimers force-firing " +
        std::to_string(keys.size()) + " timer(s)");
    for (auto& k : keys) {
        // A timer fired earlier in this loop may already have erased/replaced
        // this key (e.g. a chained reset_quest); only fire keys still pending.
        if (timers_.find(k) == timers_.end()) continue;
        timers_.erase(k);
        nlohmann::json filter;
        filter["key"] = k;
        fireTriggers("timer", filter);
        if (st_.terminated) break;
    }
}

// ---- actions (SPEC §4.2) ----

void QuestEngine::runActions(const nlohmann::json& actions) {
    if (!actions.is_array()) {
        log(questId() + ": action list is not an array: " + actions.dump());
        return;
    }
    for (auto& a : actions) {
        runAction(a);
        if (st_.terminated) break;  // terminated quest stops responding
    }
}

void QuestEngine::runAction(const nlohmann::json& action) {
    if (!action.is_object() || action.empty()) {
        log("malformed action (expected a single-verb object), got: " + action.dump());
        return;
    }
    auto it = action.begin();
    const std::string verb = it.key();
    const nlohmann::json& p = it.value();
    const std::string qid = questId();

    if (verb == "set_var") {
        std::string name;
        if (!tryGetString(p, "var", name)) {
            log(qid + ": set_var missing string 'var': " + p.dump());
            return;
        }
        const nlohmann::json* val = jsonAt(p, "value");
        if (!val || !(val->is_number() || val->is_boolean() || val->is_string())) {
            log(qid + ": set_var '" + name + "' missing scalar 'value': " + p.dump());
            return;
        }
        setVar(name, jsonToValue(*val));
    } else if (verb == "add_var") {
        std::string name;
        double delta;
        if (!tryGetString(p, "var", name)) {
            log(qid + ": add_var missing string 'var': " + p.dump());
            return;
        }
        if (!tryGetNumber(p, "value", delta)) {
            log(qid + ": add_var '" + name + "' missing number 'value': " + p.dump());
            return;
        }
        addVar(name, delta);
    } else if (verb == "set_objective_active") {
        std::string o;
        if (!tryGetIdString(p, o)) { log(qid + ": set_objective_active expects an id string"); return; }
        st_.objectives[o] = "active";
    } else if (verb == "complete_objective") {
        std::string o;
        if (!tryGetIdString(p, o)) { log(qid + ": complete_objective expects an id string"); return; }
        st_.objectives[o] = "done";
        nlohmann::json f;
        f["objective"] = o;
        fireTriggers("objective_completed", f);  // SPEC §4.2: emit on done
    } else if (verb == "fail_objective") {
        std::string o;
        if (!tryGetIdString(p, o)) { log(qid + ": fail_objective expects an id string"); return; }
        st_.objectives[o] = "failed";
    } else if (verb == "complete_quest") {
        st_.terminated = true;
    } else if (verb == "fail_quest") {
        st_.terminated = true;
    } else if (verb == "start_dialogue") {
        std::string id;
        if (!tryGetIdString(p, id)) { log(qid + ": start_dialogue expects an id string"); return; }
        startDialogue(id);
    } else if (verb == "show_message") {
        std::string text;
        if (p.is_string()) {
            text = p.get<std::string>();
        } else if (!tryGetString(p, "text", text)) {
            log(qid + ": show_message expects a string or {text}: " + p.dump());
            return;
        }
        if (d_.presenter) d_.presenter->showMessage(text);
    } else if (verb == "schedule") {
        std::string key;
        if (!tryGetString(p, "key", key)) {
            log(qid + ": schedule missing string 'key': " + p.dump());
            return;
        }
        double atVal, afterVal;
        const bool hasAt = tryGetNumber(p, "at", atVal);
        const bool hasAfter = tryGetNumber(p, "after_hours", afterVal);
        if (!hasAt && !hasAfter) {
            log(qid + ": schedule '" + key + "' needs 'at' or 'after_hours': " + p.dump());
            return;
        }
        if (!d_.clock)
            log(qid + ": schedule '" + key + "' but no Clock port (timer can't fire)");
        const double base = d_.clock ? d_.clock->gameHours() : 0.0;
        const double due = hasAt ? atVal : base + afterVal;
        timers_[key] = due;  // re-schedule replaces (SPEC §8)
    } else if (verb == "reset_quest") {
        applyInitialState();
        runStart();
    } else {
        // adapter-extension action (SPEC §4.4 / §5.2)
        if (d_.actionRunner) {
            d_.actionRunner->run(verb, p);
        } else {
            log(qid + ": unknown action '" + verb + "' and no ActionRunner -> skipped");
        }
    }
}

// ---- conditions (SPEC §4.1) ----

bool QuestEngine::evalCondition(const nlohmann::json& cond) {
    // SPEC §8: an unevaluable condition reached at runtime SHOULD be false, not
    // a crash. A non-object or empty condition is malformed -> treat as false.
    if (!cond.is_object()) {
        log("condition is not an object -> false: " + cond.dump());
        return false;
    }
    const std::string qid = questId();
    bool result = true;  // multiple keys = AND (SPEC §4.1)
    for (auto& [key, val] : cond.items()) {
        bool r = true;
        if (key == "var_eq" || key == "var_neq") {
            std::string name;
            if (!val.is_object() || !tryGetString(val, "var", name) || jsonAt(val, "value") == nullptr) {
                log(qid + ": " + key + " malformed (needs {var,value}) -> false: " + val.dump());
                r = false;
            } else {
                const bool eq = valueEq(getVar(name), jsonToValue(*jsonAt(val, "value")));
                r = (key == "var_eq") ? eq : !eq;
            }
        } else if (key == "var_gte" || key == "var_lte") {
            std::string name;
            double rhs;
            if (!val.is_object() || !tryGetString(val, "var", name) || !tryGetNumber(val, "value", rhs)) {
                log(qid + ": " + key + " malformed (needs {var, number value}) -> false: " + val.dump());
                r = false;
            } else {
                const double lhs = asDouble(getVar(name));
                r = (key == "var_gte") ? (lhs >= rhs) : (lhs <= rhs);
            }
        } else if (key == "objective_state") {
            std::string o, want;
            if (!val.is_object() || !tryGetString(val, "objective", o) || !tryGetString(val, "state", want)) {
                log(qid + ": objective_state malformed (needs {objective,state}) -> false: " + val.dump());
                r = false;
            } else {
                auto it = st_.objectives.find(o);
                r = it != st_.objectives.end() && it->second == want;
            }
        } else if (key == "all") {
            if (!val.is_array()) { log(qid + ": 'all' expects an array -> false"); r = false; }
            else {
                r = true;
                for (auto& c : val) {
                    if (!evalCondition(c)) { r = false; break; }
                }
            }
        } else if (key == "any") {
            if (!val.is_array()) { log(qid + ": 'any' expects an array -> false"); r = false; }
            else {
                r = false;
                for (auto& c : val) {
                    if (evalCondition(c)) { r = true; break; }
                }
            }
        } else if (key == "not") {
            r = !evalCondition(val);
        } else if (key == "random") {
            // SPEC §8 + §9.1: random must not be implemented until the PRF is pinned.
            log(qid + ": 'random' not implemented (PRF undefined, SPEC §9.1) -> false");
            r = false;
        } else {
            // adapter-extension condition (SPEC §4.4 / §5.3)
            if (d_.condEval) {
                r = d_.condEval->evaluate(key, val);
            } else {
                log(qid + ": unknown condition '" + key + "' and no evaluator -> false");
                r = false;
            }
        }
        if (!r) result = false;
    }
    return result;
}

// ---- dialogue flow (SPEC §3.2), resumable: present a node and yield control;
// the adapter calls submitChoice() once the player picks (presentNode never
// blocks, so this maps onto Skyrim's async MessageBox without freezing). ----

void QuestEngine::startDialogue(const std::string& id) {
    const std::string qid = questId();
    const nlohmann::json* dialogues = jsonAt(doc_, "dialogues");
    if (!dialogues || !dialogues->contains(id)) {
        log(qid + ": start_dialogue unknown dialogue '" + id + "'");  // SPEC §8 safe abort
        return;
    }
    if (!st_.activeDialogue.empty()) {
        log(qid + ": start_dialogue '" + id + "' but dialogue '" + st_.activeDialogue +
            "' is already active");  // SPEC §3.2: at most one
        return;
    }
    st_.activeDialogue = id;
    st_.currentNode = (*dialogues)[id].value("entry", std::string{});
    if (st_.currentNode.empty())
        log(qid + ": dialogue '" + id + "' has empty/missing 'entry'");
    presentCurrentNode();
}

void QuestEngine::presentCurrentNode() {
    awaitingChoice_ = false;
    pending_.clear();

    const std::string qid = questId();
    const std::string dlgId = st_.activeDialogue;
    if (dlgId.empty()) return;
    const nlohmann::json* dlg = jsonAt(doc_["dialogues"], dlgId.c_str());
    const nlohmann::json* nodesPtr = dlg ? jsonAt(*dlg, "nodes") : nullptr;
    if (!nodesPtr) {  // structurally broken dialogue reached at runtime
        log(qid + ": dialogue '" + dlgId + "' has no 'nodes' object");
        endDialogue(dlgId);
        return;
    }
    const nlohmann::json& nodes = *nodesPtr;
    const std::string node = st_.currentNode;
    if (node.empty()) { endDialogue(dlgId); return; }   // goto "" -> end
    if (!nodes.contains(node)) {
        log(qid + ": dialogue '" + dlgId + "' missing node '" + node + "'");
        endDialogue(dlgId);
        return;
    }

    const nlohmann::json& n = nodes[node];
    if (!n.is_object()) {
        log(qid + ": dialogue '" + dlgId + "' node '" + node + "' is not an object");
        endDialogue(dlgId);
        return;
    }
    const std::string speaker = n.value("speaker", "");
    std::vector<std::string> lines;
    if (const nlohmann::json* ls = jsonAt(n, "lines"); ls && ls->is_array())
        for (auto& l : *ls)
            lines.push_back(l.is_string() ? l.get<std::string>() : l.dump());

    // Build the visible choices (SPEC §3.4 filtering) up front.
    std::vector<std::string> choiceTexts;
    if (const nlohmann::json* cs = jsonAt(n, "choices"); cs && cs->is_array()) {
        for (auto& c : *cs) {
            if (!c.is_object()) {
                log(qid + ": dialogue '" + dlgId + "' node '" + node + "' has a non-object choice -> skipped");
                continue;
            }
            if (c.contains("when") && !evalCondition(c["when"])) continue;
            PendingChoice pc;
            if (c.contains("then")) pc.then = c["then"];
            pc.gotoNode = c.value("goto", std::string{});
            pc.end = c.value("end", false);
            pending_.push_back(std::move(pc));
            choiceTexts.push_back(c.value("text", ""));
        }
    }

    // Terminal node, or a node whose choices all filtered out: show + end.
    const bool terminal = n.value("end", false) || pending_.empty();
    if (terminal) {
        if (d_.presenter) d_.presenter->presentNode(speaker, lines, {});
        endDialogue(dlgId);
        return;
    }

    awaitingChoice_ = true;
    if (d_.presenter) d_.presenter->presentNode(speaker, lines, choiceTexts);
}

void QuestEngine::submitChoice(int idx) {
    if (!awaitingChoice_) {
        log("submitChoice called but no choice is awaited");
        return;
    }
    awaitingChoice_ = false;
    const std::string dlgId = st_.activeDialogue;

    if (idx < 0 || idx >= static_cast<int>(pending_.size())) {
        pending_.clear();
        endDialogue(dlgId);  // cancel / out of range -> end (SPEC §3.2)
        return;
    }

    PendingChoice pc = std::move(pending_[static_cast<std::size_t>(idx)]);
    pending_.clear();
    if (!pc.then.is_null()) runActions(pc.then);

    // A choice action may have ended the quest (complete/fail_quest) or reset it
    // (reset_quest clears st_, including activeDialogue, and re-emits quest_start).
    // Either lifecycle change supersedes the OLD dialogue, so we stop driving it.
    // BUT: reset_quest's on_start (or a dialogue_end/quest_start trigger) may have
    // opened a BRAND NEW dialogue, which already set st_.activeDialogue (to a
    // non-empty id != dlgId) and called presentCurrentNode() (rebuilding pending_/
    // awaitingChoice_). We must NOT clobber that freshly-started dialogue.
    if (st_.terminated || st_.activeDialogue != dlgId) {
        // Only clear the dialogue position when nothing fresh is in progress:
        // terminated, or the old dialogue ended without a replacement (empty). If
        // a new dialogue is active, leave its state intact.
        if (st_.terminated || st_.activeDialogue.empty()) {
            st_.activeDialogue.clear();
            st_.currentNode.clear();
        }
        return;
    }

    if (pc.end) { endDialogue(dlgId); return; }
    st_.currentNode = pc.gotoNode;  // "" -> presentCurrentNode ends it
    presentCurrentNode();
}

void QuestEngine::endDialogue(const std::string& id) {
    st_.activeDialogue.clear();
    st_.currentNode.clear();
    pending_.clear();
    awaitingChoice_ = false;
    nlohmann::json f;
    f["dialogue"] = id;
    fireTriggers("dialogue_end", f);  // may start a new dialogue (state now clear)
}

// ---- progress (de)serialization (SPEC §6) ----
//
// The blob is a flat, self-describing JSON object so a host can drop it straight
// into a co-save (the Skyrim adapter length-prefixes it as a string). It carries
// a "_v" schema version for migration (§6 "blob 內含 schema/格式版本"). Globals
// live at the system level and are handled by serializeGlobals/restoreGlobals
// below — NOT in here, so reset_quest semantics and cross-quest sharing hold.

namespace {
// Bump if the progress layout changes incompatibly. importProgress tolerates a
// different value (it reads field-by-field, defaulting anything absent), so this
// is informational/diagnostic rather than a hard gate.
constexpr int kProgressBlobVersion = 1;
}  // namespace

nlohmann::json QuestEngine::exportProgress() const {
    nlohmann::json blob = nlohmann::json::object();
    blob["_v"] = kProgressBlobVersion;
    blob["quest_id"] = questId();  // diagnostic; the host keys by quest id anyway

    nlohmann::json vars = nlohmann::json::object();
    for (const auto& [k, v] : st_.vars) vars[k] = valueToJson(v);
    blob["vars"] = std::move(vars);

    nlohmann::json objs = nlohmann::json::object();
    for (const auto& [k, s] : st_.objectives) objs[k] = s;
    blob["objectives"] = std::move(objs);

    // Current dialogue node (if a dialogue is in progress, §6). On import we
    // re-present this node, which rebuilds the pending-choice list + awaiting
    // flag from the (re-loaded) definition — so we only persist the *position*,
    // not the volatile PendingChoice cache.
    blob["active_dialogue"] = st_.activeDialogue;
    blob["current_node"] = st_.currentNode;

    blob["terminated"] = st_.terminated;

    // Whether on_start has actually run (§6: a save made BEFORE on_start ran must
    // come back un-started so the host runs start(); a save made after must not
    // re-run on_start). Distinct from "a blob exists": this is the real lifecycle.
    blob["started"] = started_;

    // Pending timers: key -> ABSOLUTE due game-hours (§6/§8 "以絕對遊戲時間存").
    nlohmann::json timers = nlohmann::json::object();
    for (const auto& [k, due] : timers_) timers[k] = due;
    blob["timers"] = std::move(timers);

    return blob;
}

bool QuestEngine::importProgress(const nlohmann::json& blob) {
    if (!blob.is_object()) {
        log(questId() + ": importProgress got a non-object blob -> ignored");
        return false;
    }

    // Diagnostic only: read but do not gate on the version (we degrade per-field).
    if (const nlohmann::json* v = jsonAt(blob, "_v"); v && v->is_number_integer() &&
        v->get<int>() != kProgressBlobVersion) {
        log(questId() + ": importProgress blob version " + std::to_string(v->get<int>()) +
            " != " + std::to_string(kProgressBlobVersion) + " (loading field-by-field)");
    }

    // Layer onto a CLEAN initial state so a partial blob can't leave stale
    // entries from a prior start()/import. applyInitialState() also clears
    // timers_, pending_ and awaitingChoice_ (§6 "定義與進度分離").
    applyInitialState();

    if (const nlohmann::json* vars = jsonAt(blob, "vars"); vars && vars->is_object()) {
        for (auto& [k, v] : vars->items()) {
            if (v.is_number() || v.is_boolean() || v.is_string())
                st_.vars[k] = jsonToValue(v);  // overrides the JSON-default initial value
        }
    }

    if (const nlohmann::json* objs = jsonAt(blob, "objectives"); objs && objs->is_object()) {
        for (auto& [k, s] : objs->items())
            if (s.is_string()) st_.objectives[k] = s.get<std::string>();
    }

    if (const nlohmann::json* t = jsonAt(blob, "terminated"); t && t->is_boolean())
        st_.terminated = t->get<bool>();

    // Restore the persisted lifecycle flag. applyInitialState() above already set
    // started_ = false, so a missing/non-bool field (an OLD save written before
    // this field existed, or a save made before on_start ran) correctly stays
    // false — the host then runs start() on it. Only an after-on_start save flips
    // it true and thereby suppresses a re-run of on_start.
    if (const nlohmann::json* s = jsonAt(blob, "started"); s && s->is_boolean())
        started_ = s->get<bool>();

    if (const nlohmann::json* timers = jsonAt(blob, "timers"); timers && timers->is_object()) {
        for (auto& [k, due] : timers->items())
            if (due.is_number()) timers_[k] = due.get<double>();
    }

    // Restore dialogue position last. We re-present the node so the presenter
    // shows it again and pending_/awaitingChoice_ are rebuilt from the reloaded
    // definition (§6 resumable dialogue / §4.5 still-awaiting message). Guard the
    // node against a definition that changed under it (presentCurrentNode logs +
    // ends safely if the node went missing — §8 safe abort, no crash).
    std::string activeDlg, curNode;
    if (const nlohmann::json* a = jsonAt(blob, "active_dialogue"); a && a->is_string())
        activeDlg = a->get<std::string>();
    if (const nlohmann::json* c = jsonAt(blob, "current_node"); c && c->is_string())
        curNode = c->get<std::string>();
    if (!st_.terminated && !activeDlg.empty()) {
        const nlohmann::json* dialogues = jsonAt(doc_, "dialogues");
        if (dialogues && dialogues->contains(activeDlg)) {
            st_.activeDialogue = activeDlg;
            st_.currentNode = curNode;
            presentCurrentNode();  // rebuilds pending_/awaitingChoice_, or ends safely
        } else {
            log(questId() + ": importProgress saved dialogue '" + activeDlg +
                "' no longer exists in the definition -> dialogue dropped");
        }
    }
    return true;
}

// ---- system-level globals (SPEC §2.4 / §6, persisted apart from quest progress) ----

nlohmann::json serializeGlobals(const GlobalStore& globals) {
    nlohmann::json out = nlohmann::json::object();
    for (const auto& [k, v] : globals.vars) out[k] = valueToJson(v);
    return out;
}

void restoreGlobals(GlobalStore& globals, const nlohmann::json& blob) {
    if (!blob.is_object()) return;  // tolerant: nothing to restore
    for (auto& [k, v] : blob.items()) {
        if (!(v.is_number() || v.is_boolean() || v.is_string())) continue;
        auto it = globals.vars.find(k);
        if (it != globals.vars.end()) {
            // Keep the host-declared TYPE stable across reload (§7.6): only take
            // the stored value if it matches the declared alternative's type;
            // otherwise leave the declared default in place.
            const Value incoming = jsonToValue(v);
            if (incoming.index() == it->second.index()) it->second = incoming;
        } else {
            // Not declared by the host (yet) — import it anyway so a global the
            // host adds later isn't silently lost from an existing save.
            globals.vars[k] = jsonToValue(v);
        }
    }
}

// ---- offline validation (SPEC §7) ----

namespace {

// One JSON-scalar type tag, for the declared-type check (SPEC §8 type match).
enum class VType { Number, Bool, String, Other };
VType vtypeOf(const nlohmann::json& j) {
    if (j.is_boolean()) return VType::Bool;
    if (j.is_number()) return VType::Number;
    if (j.is_string()) return VType::String;
    return VType::Other;
}
const char* vtypeName(VType t) {
    switch (t) {
        case VType::Number: return "number";
        case VType::Bool:   return "bool";
        case VType::String: return "string";
        default:            return "other";
    }
}

// The set of recognised core condition keys. Anything else is treated as an
// adapter extension and is NOT flagged by core validation (SPEC §4.4). (Action
// verbs are matched inline in checkAction; unrecognised verbs are extensions.)
bool isCoreConditionKey(const std::string& k) {
    static const std::vector<std::string> c = {
        "var_eq", "var_neq", "var_gte", "var_lte", "objective_state",
        "random", "all", "any", "not"};
    for (auto& s : c) if (s == k) return true;
    return false;
}
bool isValidObjectiveState(const std::string& s) {
    return s == "inactive" || s == "active" || s == "done" || s == "failed";
}
}  // namespace

std::vector<std::string> QuestEngine::validate() const {
    std::vector<std::string> errs;
    const std::string qid = doc_.is_object() ? doc_.value("id", std::string{}) : std::string{};
    const std::string tag = "[" + (qid.empty() ? std::string{"<no id>"} : qid) + "] ";
    auto err = [&](const std::string& path, const std::string& msg) {
        errs.push_back(tag + path + ": " + msg);
    };

    if (!doc_.is_object()) {
        errs.push_back("quest document is not a JSON object");
        return errs;
    }

    // ---- top level (SPEC §2) ----
    if (qid.empty()) err("id", "missing or non-string required field 'id'");
    if (!doc_.contains("title") || !doc_["title"].is_string())
        err("title", "missing or non-string required field 'title'");
    if (const auto* v = jsonAt(doc_, "version"); v && !v->is_number_integer())
        err("version", "must be an integer");
    if (const auto* p = jsonAt(doc_, "priority"); p) {
        const std::string s = p->is_string() ? p->get<std::string>() : "";
        if (s != "high" && s != "normal" && s != "low")
            err("priority", "must be one of high|normal|low");
    }

    // ---- collect declarations (vars / objectives / dialogues) ----
    std::map<std::string, VType> varTypes;   // quest-scoped var -> declared type
    if (const auto* vars = jsonAt(doc_, "vars")) {
        if (!vars->is_object()) err("vars", "must be an object");
        else for (auto& [k, v] : vars->items()) {
            if (vtypeOf(v) == VType::Other)
                err("vars." + k, "value must be number / bool / string");
            else
                varTypes[k] = vtypeOf(v);
        }
    }
    std::vector<std::string> objIds;
    if (const auto* objs = jsonAt(doc_, "objectives")) {
        if (!objs->is_object()) err("objectives", "must be an object");
        else for (auto& [k, o] : objs->items()) {
            objIds.push_back(k);
            if (!o.is_object()) { err("objectives." + k, "must be an object"); continue; }
            if (!o.contains("text") || !o["text"].is_string())
                err("objectives." + k + ".text", "missing or non-string required 'text'");
            if (const auto* s = jsonAt(o, "state");
                s && (!s->is_string() || !isValidObjectiveState(s->get<std::string>())))
                err("objectives." + k + ".state", "must be inactive|active|done|failed");
        }
    }
    auto objExists = [&](const std::string& id) {
        for (auto& o : objIds) if (o == id) return true;
        return false;
    };

    std::vector<std::string> dlgIds;
    if (const auto* dlgs = jsonAt(doc_, "dialogues")) {
        if (!dlgs->is_object()) err("dialogues", "must be an object");
        else for (auto& [k, _] : dlgs->items()) dlgIds.push_back(k);
    }
    auto dlgExists = [&](const std::string& id) {
        for (auto& d : dlgIds) if (d == id) return true;
        return false;
    };

    // ---- a var reference: type check + declared check (SPEC §7.6 globals) ----
    auto checkVarRef = [&](const std::string& path, const std::string& name,
                           const nlohmann::json* valueOrNull) {
        if (isGlobal(name)) {
            const std::string g = name.substr(kGlobalPrefixLen);
            if (!d_.globals || d_.globals->vars.find(g) == d_.globals->vars.end()) {
                err(path, "references undeclared global var '" + name +
                          "' (SPEC §2.4/§7.6)");
                return;
            }
            if (valueOrNull && vtypeOf(*valueOrNull) != VType::Other) {
                const Value& cur = d_.globals->vars.at(g);
                VType decl = std::holds_alternative<double>(cur) ? VType::Number
                            : std::holds_alternative<bool>(cur) ? VType::Bool
                                                                : VType::String;
                if (decl != vtypeOf(*valueOrNull))
                    err(path, "type mismatch: global '" + name + "' is " +
                              vtypeName(decl) + ", value is " + vtypeName(vtypeOf(*valueOrNull)));
            }
            return;
        }
        auto it = varTypes.find(name);
        if (it == varTypes.end()) {
            err(path, "references undeclared var '" + name + "' (not in 'vars')");
            return;
        }
        if (valueOrNull && vtypeOf(*valueOrNull) != VType::Other &&
            it->second != vtypeOf(*valueOrNull)) {
            err(path, "type mismatch: var '" + name + "' is " + vtypeName(it->second) +
                      ", value is " + vtypeName(vtypeOf(*valueOrNull)) + " (SPEC §8)");
        }
    };

    // ---- recursive condition validator ----
    std::function<void(const std::string&, const nlohmann::json&)> checkCond =
        [&](const std::string& path, const nlohmann::json& cond) {
            if (!cond.is_object() || cond.empty()) {
                err(path, "condition must be a non-empty object");
                return;
            }
            for (auto& [k, v] : cond.items()) {
                const std::string p = path + "." + k;
                if (k == "var_eq" || k == "var_neq" || k == "var_gte" || k == "var_lte") {
                    if (!v.is_object() || !v.contains("var") || !v["var"].is_string() ||
                        !v.contains("value")) {
                        err(p, "needs {var:string, value}");
                        continue;
                    }
                    const bool numericOnly = (k == "var_gte" || k == "var_lte");
                    if (numericOnly && !v["value"].is_number())
                        err(p, "value must be a number for " + k);
                    checkVarRef(p, v["var"].get<std::string>(),
                                numericOnly ? nullptr : &v["value"]);
                } else if (k == "objective_state") {
                    if (!v.is_object() || !v.contains("objective") || !v["objective"].is_string() ||
                        !v.contains("state") || !v["state"].is_string()) {
                        err(p, "needs {objective:string, state:string}");
                        continue;
                    }
                    if (!objExists(v["objective"].get<std::string>()))
                        err(p, "references unknown objective '" + v["objective"].get<std::string>() + "'");
                    if (!isValidObjectiveState(v["state"].get<std::string>()))
                        err(p, "state must be inactive|active|done|failed");
                } else if (k == "all" || k == "any") {
                    if (!v.is_array()) { err(p, "must be an array of conditions"); continue; }
                    for (std::size_t i = 0; i < v.size(); ++i)
                        checkCond(p + "[" + std::to_string(i) + "]", v[i]);
                } else if (k == "not") {
                    checkCond(p, v);
                } else if (k == "random") {
                    err(p, "'random' is not implemented (PRF undefined, SPEC §9.1)");
                } else if (!isCoreConditionKey(k)) {
                    // adapter-extension condition (§4.4) — core does not judge it.
                }
            }
        };

    // ---- action validator. dlgRefs collects start_dialogue targets so we can
    // also flag dialogues that are declared but never reachable is NOT required;
    // we only flag references to *missing* dialogues. ----
    auto checkAction = [&](const std::string& path, const nlohmann::json& a) {
        if (!a.is_object() || a.size() != 1) {
            err(path, "action must be an object with exactly one verb key");
            return;
        }
        const std::string verb = a.begin().key();
        const nlohmann::json& p = a.begin().value();
        const std::string vp = path + "." + verb;
        if (verb == "set_var") {
            if (!p.is_object() || !p.contains("var") || !p["var"].is_string() || !p.contains("value"))
                err(vp, "needs {var:string, value}");
            else if (vtypeOf(p["value"]) == VType::Other)
                err(vp, "value must be number / bool / string");
            else
                checkVarRef(vp, p["var"].get<std::string>(), &p["value"]);
        } else if (verb == "add_var") {
            if (!p.is_object() || !p.contains("var") || !p["var"].is_string() ||
                !p.contains("value") || !p["value"].is_number())
                err(vp, "needs {var:string, value:number}");
            else
                checkVarRef(vp, p["var"].get<std::string>(), nullptr);  // numeric op
        } else if (verb == "set_objective_active" || verb == "complete_objective" ||
                   verb == "fail_objective") {
            if (!p.is_string()) err(vp, "expects an objective id string");
            else if (!objExists(p.get<std::string>()))
                err(vp, "references unknown objective '" + p.get<std::string>() + "'");
        } else if (verb == "start_dialogue") {
            if (!p.is_string()) err(vp, "expects a dialogue id string");
            else if (!dlgExists(p.get<std::string>()))
                err(vp, "references unknown dialogue '" + p.get<std::string>() + "'");
        } else if (verb == "complete_quest" || verb == "fail_quest" || verb == "reset_quest") {
            if (!(p.is_boolean() && p.get<bool>())) err(vp, "expects true");
        } else if (verb == "show_message") {
            if (!p.is_string() && !(p.is_object() && p.contains("text") && p["text"].is_string()))
                err(vp, "expects a string or {text:string}");
        } else if (verb == "schedule") {
            if (!p.is_object() || !p.contains("key") || !p["key"].is_string())
                err(vp, "needs {key:string}");
            const bool hasAt = p.is_object() && p.contains("at") && p["at"].is_number();
            const bool hasAfter = p.is_object() && p.contains("after_hours") && p["after_hours"].is_number();
            if (!hasAt && !hasAfter)
                err(vp, "needs numeric 'at' or 'after_hours'");
            if (!d_.clock)
                err(vp, "uses 'schedule' but no Clock port is wired (SPEC §5.7) — timer can't fire");
        }
        // else: adapter-extension verb (§4.4) — not judged by the core.
    };

    auto checkActionList = [&](const std::string& path, const nlohmann::json& list) {
        if (!list.is_array()) { err(path, "must be an array of actions"); return; }
        for (std::size_t i = 0; i < list.size(); ++i)
            checkAction(path + "[" + std::to_string(i) + "]", list[i]);
    };

    // ---- on_start ----
    if (const auto* os = jsonAt(doc_, "on_start"))
        checkActionList("on_start", *os);

    // ---- triggers (SPEC §2.3 / §4.3) ----
    if (const auto* trigs = jsonAt(doc_, "triggers")) {
        if (!trigs->is_array()) err("triggers", "must be an array");
        else for (std::size_t i = 0; i < trigs->size(); ++i) {
            const std::string tp = "triggers[" + std::to_string(i) + "]";
            const nlohmann::json& t = (*trigs)[i];
            if (!t.is_object()) { err(tp, "must be an object"); continue; }
            if (!t.contains("on") || !t["on"].is_string())
                err(tp + ".on", "missing or non-string required 'on'");
            if (!t.contains("do")) err(tp + ".do", "missing required 'do' action list");
            else checkActionList(tp + ".do", t["do"]);
            if (t.contains("when")) checkCond(tp + ".when", t["when"]);
            // Core-event filter sanity: objective_completed -> objective must exist.
            if (t.value("on", std::string{}) == "objective_completed" && t.contains("objective") &&
                t["objective"].is_string() && !objExists(t["objective"].get<std::string>()))
                err(tp + ".objective", "filters on unknown objective '" +
                                       t["objective"].get<std::string>() + "'");
            if (t.value("on", std::string{}) == "dialogue_end" && t.contains("dialogue") &&
                t["dialogue"].is_string() && !dlgExists(t["dialogue"].get<std::string>()))
                err(tp + ".dialogue", "filters on unknown dialogue '" +
                                      t["dialogue"].get<std::string>() + "'");
        }
    }

    // ---- dialogues / nodes / choices (SPEC §2.1-2.2, §8 goto target) ----
    if (const auto* dlgs = jsonAt(doc_, "dialogues"); dlgs && dlgs->is_object()) {
        for (auto& [did, dlg] : dlgs->items()) {
            const std::string dp = "dialogues." + did;
            if (!dlg.is_object()) { err(dp, "must be an object"); continue; }
            const nlohmann::json* nodes = jsonAt(dlg, "nodes");
            if (!nodes || !nodes->is_object()) { err(dp + ".nodes", "missing or non-object 'nodes'"); }
            std::string entry = dlg.value("entry", std::string{});
            if (entry.empty()) err(dp + ".entry", "missing or empty required 'entry'");
            else if (nodes && nodes->is_object() && !nodes->contains(entry))
                err(dp + ".entry", "'entry' points to missing node '" + entry + "'");
            if (!nodes || !nodes->is_object()) continue;

            for (auto& [nid, n] : nodes->items()) {
                const std::string np = dp + ".nodes." + nid;
                if (!n.is_object()) { err(np, "must be an object"); continue; }
                if (!n.contains("speaker") || !n["speaker"].is_string())
                    err(np + ".speaker", "missing or non-string required 'speaker'");
                const bool hasChoices = n.contains("choices");
                const bool isEnd = n.value("end", false);
                if (!hasChoices && !isEnd)
                    err(np, "node needs either 'choices' or 'end:true'");
                if (hasChoices) {
                    if (!n["choices"].is_array()) err(np + ".choices", "must be an array");
                    else {
                        const auto& cs = n["choices"];
                        for (std::size_t ci = 0; ci < cs.size(); ++ci) {
                            const std::string cp = np + ".choices[" + std::to_string(ci) + "]";
                            const nlohmann::json& c = cs[ci];
                            if (!c.is_object()) { err(cp, "must be an object"); continue; }
                            if (!c.contains("text") || !c["text"].is_string())
                                err(cp + ".text", "missing or non-string required 'text'");
                            const bool cGoto = c.contains("goto");
                            const bool cEnd = c.value("end", false);
                            if (!cGoto && !cEnd)
                                err(cp, "choice needs either 'goto' or 'end:true'");
                            if (cGoto) {
                                if (!c["goto"].is_string()) err(cp + ".goto", "must be a string");
                                else if (!nodes->contains(c["goto"].get<std::string>()))
                                    err(cp + ".goto", "points to missing node '" +
                                                      c["goto"].get<std::string>() + "' (SPEC §8)");
                            }
                            if (c.contains("when")) checkCond(cp + ".when", c["when"]);
                            if (c.contains("then")) checkActionList(cp + ".then", c["then"]);
                        }
                    }
                }
            }
        }
    }

    return errs;
}

}  // namespace qe
