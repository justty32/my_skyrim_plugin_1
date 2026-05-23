#include "QuestEngine.h"

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

bool isGlobal(const std::string& name) {
    return name.rfind(kGlobalPrefix, 0) == 0;
}
}  // namespace

QuestEngine::QuestEngine(nlohmann::json doc, Deps deps)
    : doc_(std::move(doc)), d_(deps) {}

void QuestEngine::log(const std::string& msg) const {
    if (d_.logger) d_.logger->log(msg);
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
    if (doc_.contains("vars")) {
        for (auto& [k, v] : doc_["vars"].items()) st_.vars[k] = jsonToValue(v);
    }
    if (doc_.contains("objectives")) {
        for (auto& [k, o] : doc_["objectives"].items())
            st_.objectives[k] = o.value("state", "inactive");
    }
}

void QuestEngine::runStart() {
    if (doc_.contains("on_start")) runActions(doc_["on_start"]);
    fireTriggers("quest_start", nlohmann::json::object());
}

void QuestEngine::start() {
    applyInitialState();
    runStart();
}

// ---- triggers (SPEC §3.3) ----

void QuestEngine::dispatchEvent(const std::string& on, const nlohmann::json& filter) {
    fireTriggers(on, filter);
}

bool QuestEngine::triggerMatches(const nlohmann::json& trig, const std::string& on,
                                 const nlohmann::json& filter) const {
    if (trig.value("on", std::string{}) != on) return false;
    // Filter keys that, when present on the trigger, must equal the event's.
    static const char* kFilterKeys[] = {"dialogue", "objective", "character",
                                         "form", "location", "key"};
    for (const char* fk : kFilterKeys) {
        if (trig.contains(fk)) {
            if (!filter.contains(fk) || filter[fk] != trig[fk]) return false;
        }
    }
    return true;
}

void QuestEngine::fireTriggers(const std::string& on, const nlohmann::json& filter) {
    if (!doc_.contains("triggers")) return;
    for (auto& trig : doc_["triggers"]) {
        if (!triggerMatches(trig, on, filter)) continue;
        if (trig.contains("when") && !evalCondition(trig["when"])) continue;
        if (trig.contains("do")) runActions(trig["do"]);
        if (st_.terminated) break;
    }
}

void QuestEngine::checkTimers() {
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

// ---- actions (SPEC §4.2) ----

void QuestEngine::runActions(const nlohmann::json& actions) {
    for (auto& a : actions) {
        runAction(a);
        if (st_.terminated) break;  // terminated quest stops responding
    }
}

void QuestEngine::runAction(const nlohmann::json& action) {
    if (!action.is_object() || action.empty()) {
        log("malformed action (not a single-verb object)");
        return;
    }
    auto it = action.begin();
    const std::string verb = it.key();
    const nlohmann::json& p = it.value();

    if (verb == "set_var") {
        setVar(p["var"].get<std::string>(), jsonToValue(p["value"]));
    } else if (verb == "add_var") {
        addVar(p["var"].get<std::string>(), p["value"].get<double>());
    } else if (verb == "set_objective_active") {
        st_.objectives[p.get<std::string>()] = "active";
    } else if (verb == "complete_objective") {
        const std::string o = p.get<std::string>();
        st_.objectives[o] = "done";
        nlohmann::json f;
        f["objective"] = o;
        fireTriggers("objective_completed", f);
    } else if (verb == "fail_objective") {
        st_.objectives[p.get<std::string>()] = "failed";
    } else if (verb == "complete_quest") {
        st_.terminated = true;
    } else if (verb == "fail_quest") {
        st_.terminated = true;
    } else if (verb == "start_dialogue") {
        startDialogue(p.get<std::string>());
    } else if (verb == "show_message") {
        const std::string text = p.is_string() ? p.get<std::string>() : p.value("text", "");
        if (d_.presenter) d_.presenter->showMessage(text);
    } else if (verb == "schedule") {
        const double base = d_.clock ? d_.clock->gameHours() : 0.0;
        double due = p.contains("at") ? p["at"].get<double>()
                                      : base + p["after_hours"].get<double>();
        timers_[p["key"].get<std::string>()] = due;  // re-schedule replaces (SPEC §8)
    } else if (verb == "reset_quest") {
        applyInitialState();
        runStart();
    } else {
        // adapter-extension action (SPEC §4.4 / §5.2)
        if (d_.actionRunner) {
            d_.actionRunner->run(verb, p);
        } else {
            log("unknown action '" + verb + "' and no ActionRunner -> skipped");
        }
    }
}

// ---- conditions (SPEC §4.1) ----

bool QuestEngine::evalCondition(const nlohmann::json& cond) {
    if (!cond.is_object()) return false;
    bool result = true;  // multiple keys = AND (SPEC §4.1)
    for (auto& [key, val] : cond.items()) {
        bool r = true;
        if (key == "var_eq") {
            r = valueEq(getVar(val["var"].get<std::string>()), jsonToValue(val["value"]));
        } else if (key == "var_neq") {
            r = !valueEq(getVar(val["var"].get<std::string>()), jsonToValue(val["value"]));
        } else if (key == "var_gte") {
            r = asDouble(getVar(val["var"].get<std::string>())) >= val["value"].get<double>();
        } else if (key == "var_lte") {
            r = asDouble(getVar(val["var"].get<std::string>())) <= val["value"].get<double>();
        } else if (key == "objective_state") {
            const std::string o = val["objective"].get<std::string>();
            auto it = st_.objectives.find(o);
            r = it != st_.objectives.end() && it->second == val["state"].get<std::string>();
        } else if (key == "all") {
            r = true;
            for (auto& c : val) {
                if (!evalCondition(c)) { r = false; break; }
            }
        } else if (key == "any") {
            r = false;
            for (auto& c : val) {
                if (evalCondition(c)) { r = true; break; }
            }
        } else if (key == "not") {
            r = !evalCondition(val);
        } else if (key == "random") {
            // SPEC §8: random must not be implemented until the PRF is pinned.
            log("'random' not implemented (PRF undefined) -> false");
            r = false;
        } else {
            // adapter-extension condition (SPEC §4.4 / §5.3)
            if (d_.condEval) {
                r = d_.condEval->evaluate(key, val);
            } else {
                log("unknown condition '" + key + "' and no evaluator -> false");
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
    if (!doc_.contains("dialogues") || !doc_["dialogues"].contains(id)) {
        log("start_dialogue: unknown dialogue '" + id + "'");
        return;
    }
    if (!st_.activeDialogue.empty()) {
        log("start_dialogue: a dialogue is already active");  // SPEC §3.2: at most one
        return;
    }
    st_.activeDialogue = id;
    st_.currentNode = doc_["dialogues"][id].value("entry", std::string{});
    presentCurrentNode();
}

void QuestEngine::presentCurrentNode() {
    awaitingChoice_ = false;
    pending_.clear();

    const std::string dlgId = st_.activeDialogue;
    if (dlgId.empty()) return;
    const nlohmann::json& nodes = doc_["dialogues"][dlgId]["nodes"];
    const std::string node = st_.currentNode;
    if (node.empty()) { endDialogue(dlgId); return; }   // goto "" -> end
    if (!nodes.contains(node)) {
        log("dialogue '" + dlgId + "': missing node '" + node + "'");
        endDialogue(dlgId);
        return;
    }

    const nlohmann::json& n = nodes[node];
    const std::string speaker = n.value("speaker", "");
    std::vector<std::string> lines;
    if (n.contains("lines"))
        for (auto& l : n["lines"]) lines.push_back(l.get<std::string>());

    // Build the visible choices (SPEC §3.4 filtering) up front.
    std::vector<std::string> choiceTexts;
    if (n.contains("choices")) {
        for (auto& c : n["choices"]) {
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
    // Either lifecycle change supersedes the dialogue: just ensure it's cleared.
    if (st_.terminated || st_.activeDialogue != dlgId) {
        st_.activeDialogue.clear();
        st_.currentNode.clear();
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

}  // namespace qe
