// Headless CLI adapter for the portable quest engine (SPEC appendix A).
// Shares ZERO assumptions with Skyrim — its only job is to prove the core runs
// with no game engine, and to act as a cheap conformance / iteration harness.
//
// Build: scripts/build_cli.sh  ->  build/cli/qe_cli [quest.json]
//
// REPL commands:
//   time <hours>            advance the game clock, then fire due timers
//   cast                    fire spell_cast_on {character: victim}
//   fire <on> [k v]         fire an arbitrary event with one optional filter
//   state                   dump vars / objectives / globals / clock
//   quit | exit | <EOF>     leave
// Dialogue choices are read (1-based) from the same stdin.

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "QuestEngine.h"
#include "QuestState.h"

using nlohmann::json;

namespace {

class CliPresenter : public qe::IDialoguePresenter {
public:
    int presentNode(const std::string& speaker, const std::vector<std::string>& lines,
                    const std::vector<std::string>& choices) override {
        std::cout << "\n";
        for (const auto& l : lines) std::cout << "  " << speaker << ": " << l << "\n";
        if (choices.empty()) return -1;  // terminal node: no input consumed
        for (std::size_t i = 0; i < choices.size(); ++i)
            std::cout << "    [" << (i + 1) << "] " << choices[i] << "\n";
        std::cout << "  choose> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) return -1;
        try {
            return std::stoi(line) - 1;  // 1-based -> 0-based
        } catch (...) {
            return -1;
        }
    }
    void showMessage(const std::string& text) override {
        std::cout << "  \xC2\xBB " << text << "\n";  // » text
    }
};

class CliClock : public qe::IClock {
public:
    double hours = 0.0;
    double gameHours() override { return hours; }
};

class CliLogger : public qe::ILogger {
public:
    void log(const std::string& msg) override { std::cout << "  [log] " << msg << "\n"; }
};

class CliActionRunner : public qe::IActionRunner {
public:
    void run(const std::string& verb, const json& params) override {
        std::cout << "  [adapter action] " << verb << " " << params.dump() << "\n";
    }
};

class CliConditionEvaluator : public qe::IConditionEvaluator {
public:
    bool evaluate(const std::string& key, const json& params) override {
        std::cout << "  [adapter condition] " << key << " " << params.dump()
                  << " -> false (stub)\n";
        return false;
    }
};

void dumpState(const qe::QuestEngine& engine, const qe::GlobalStore& globals,
               const CliClock& clock) {
    const auto& st = engine.state();
    std::cout << "  --- state ---\n";
    std::cout << "  clock: " << clock.hours << "h   terminated: "
              << (engine.terminated() ? "yes" : "no") << "\n";
    std::cout << "  vars:";
    for (const auto& [k, v] : st.vars) std::cout << " " << k << "=" << qe::valueToString(v);
    std::cout << "\n  objectives:";
    for (const auto& [k, s] : st.objectives) std::cout << " " << k << "=" << s;
    std::cout << "\n  globals:";
    for (const auto& [k, v] : globals.vars)
        std::cout << " global." << k << "=" << qe::valueToString(v);
    std::cout << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    const std::string path =
        argc > 1 ? argv[1] : "config/quests/demo_court_wizard.json";

    std::ifstream in(path);
    if (!in) {
        std::cerr << "cannot open quest file: " << path << "\n";
        return 1;
    }
    json doc;
    try {
        in >> doc;
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error in " << path << ": " << e.what() << "\n";
        return 1;
    }

    CliPresenter presenter;
    CliClock clock;
    CliLogger logger;
    CliActionRunner runner;
    CliConditionEvaluator condEval;
    qe::GlobalStore globals;
    // SPEC §2.4: globals are declared up front (here, by the harness).
    globals.vars["whiterun_tasks_done"] = 0.0;

    qe::QuestEngine::Deps deps;
    deps.presenter = &presenter;
    deps.clock = &clock;
    deps.logger = &logger;
    deps.actionRunner = &runner;
    deps.condEval = &condEval;
    deps.globals = &globals;

    qe::QuestEngine engine(doc, deps);
    std::cout << "== quest: " << doc.value("title", doc.value("id", "?")) << " ==\n";
    engine.start();

    std::string line;
    std::cout << "\n(cmd: time N | cast | fire <on> [k v] | state | quit)\n> " << std::flush;
    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd.empty()) {
            // re-prompt
        } else if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "time") {
            double h = 0;
            ss >> h;
            clock.hours += h;
            engine.checkTimers();
        } else if (cmd == "cast") {
            json f;
            f["character"] = "victim";
            engine.dispatchEvent("spell_cast_on", f);
        } else if (cmd == "fire") {
            std::string on, k, v;
            ss >> on;
            json f = json::object();
            if (ss >> k >> v) f[k] = v;
            engine.dispatchEvent(on, f);
        } else if (cmd == "state") {
            dumpState(engine, globals, clock);
        } else {
            std::cout << "  ? unknown command: " << cmd << "\n";
        }
        std::cout << "> " << std::flush;
    }
    std::cout << "\nbye.\n";
    return 0;
}
