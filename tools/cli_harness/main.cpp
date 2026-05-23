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
//   save                    snapshot progress+globals into a co-save blob (§6)
//   reload                  reconstruct the engine from the definition + the blob
//   quit | exit | <EOF>     leave
// Dialogue choices are read (1-based) from the same stdin.

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "QuestEngine.h"
#include "QuestState.h"

using nlohmann::json;

namespace {

// Display-only presenter (resumable dialogue): print the node and, for a choice
// node, the prompt — but DON'T read input here. The choice is fed back via
// QuestEngine::submitChoice() from the main loop (see drainChoices).
class CliPresenter : public qe::IDialoguePresenter {
public:
    void presentNode(const std::string& speaker, const std::vector<std::string>& lines,
                     const std::vector<std::string>& choices) override {
        std::cout << "\n";
        for (const auto& l : lines) std::cout << "  " << speaker << ": " << l << "\n";
        for (std::size_t i = 0; i < choices.size(); ++i)
            std::cout << "    [" << (i + 1) << "] " << choices[i] << "\n";
        if (!choices.empty()) std::cout << "  choose> " << std::flush;
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

// While the engine is awaiting a dialogue choice, read the player's pick
// (1-based) from stdin and feed it back. Mirrors what a Skyrim adapter does in
// its async MessageBox callback, just synchronously over stdin.
void drainChoices(qe::QuestEngine& engine) {
    while (engine.awaitingChoice()) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            engine.submitChoice(-1);  // EOF -> cancel
            break;
        }
        try {
            engine.submitChoice(std::stoi(line) - 1);  // 1-based -> 0-based
        } catch (...) {
            engine.submitChoice(-1);  // non-numeric -> cancel
        }
    }
}

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
    // SPEC §2.4: globals MUST be declared up front (here, by the harness) so a
    // global.<name> reference can be validated (§7.6). A real adapter loads
    // these from a manifest; the CLI hard-codes the ones the bundled quests use.
    globals.vars["whiterun_tasks_done"] = 0.0;
    globals.vars["runs"] = 0.0;
    globals.vars["reputation"] = 0.0;

    qe::QuestEngine::Deps deps;
    deps.presenter = &presenter;
    deps.clock = &clock;
    deps.logger = &logger;
    deps.actionRunner = &runner;
    deps.condEval = &condEval;
    deps.globals = &globals;

    // Held by unique_ptr so the `reload` command can DESTROY + RECONSTRUCT it
    // from the same definition (proving SPEC §6 "定義與進度分離": reload the JSON
    // definition fresh, then layer the saved progress blob on top).
    auto enginePtr = std::make_unique<qe::QuestEngine>(doc, deps);
    const std::string title =
        doc.is_object() ? doc.value("title", doc.value("id", std::string{"?"})) : std::string{"<not a JSON object>"};
    std::cout << "== quest: " << title << " ==\n";

    // A single round-trip co-save buffer (mirrors the Skyrim adapter's 'QEST'
    // record): {progress blob} + {globals}. `save` captures it; `reload` rebuilds
    // the engine from the definition and re-applies it.
    json savedBlob;

    // SPEC §7: validate against the core vocabulary before running. The CLI is
    // an offline validator + conformance harness, so it always reports. A real
    // adapter would merge its extension schema first ("effective schema", §4.4);
    // here, any leftover problem is a genuine core-level structural error.
    const auto problems = enginePtr->validate();
    if (!problems.empty()) {
        std::cerr << "validation: " << problems.size() << " problem(s):\n";
        for (const auto& p : problems) std::cerr << "  - " << p << "\n";
        // --strict: refuse to run a structurally invalid quest.
        if (argc > 2 && std::string(argv[2]) == "--strict") {
            std::cerr << "(--strict) refusing to run.\n";
            return 2;
        }
        std::cerr << "(continuing anyway; pass --strict to abort)\n";
    }

    enginePtr->start();
    drainChoices(*enginePtr);  // on_start may open a dialogue immediately

    std::string line;
    std::cout << "\n(cmd: time N | cast | fire <on> [k v] | state | save | reload | quit)\n> " << std::flush;
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
            enginePtr->checkTimers();
        } else if (cmd == "cast") {
            json f;
            f["character"] = "victim";
            enginePtr->dispatchEvent("spell_cast_on", f);
        } else if (cmd == "fire") {
            std::string on, k, v;
            ss >> on;
            json f = json::object();
            if (ss >> k >> v) f[k] = v;
            enginePtr->dispatchEvent(on, f);
        } else if (cmd == "state") {
            dumpState(*enginePtr, globals, clock);
        } else if (cmd == "save") {
            // SPEC §6: snapshot progress + system-level globals into one blob
            // (what the Skyrim adapter writes into its 'QEST' co-save record).
            savedBlob = json::object();
            savedBlob["progress"] = enginePtr->exportProgress();
            savedBlob["globals"] = qe::serializeGlobals(globals);
            std::cout << "  [save] " << savedBlob.dump() << "\n";
        } else if (cmd == "reload") {
            // Simulate a save -> reload: tear the engine down, reconstruct it from
            // the SAME definition (a real reload re-parses the JSON), restore the
            // system globals, then layer the saved progress (§6 split). The clock
            // is left as-is (game time persists in the real save).
            if (savedBlob.is_null()) {
                std::cout << "  [reload] nothing saved yet\n";
            } else {
                enginePtr = std::make_unique<qe::QuestEngine>(doc, deps);
                qe::restoreGlobals(globals, savedBlob.value("globals", json::object()));
                enginePtr->importProgress(savedBlob.value("progress", json::object()));
                std::cout << "  [reload] restored from save\n";
            }
        } else {
            std::cout << "  ? unknown command: " << cmd << "\n";
        }
        drainChoices(*enginePtr);  // an event may have opened a dialogue
        std::cout << "> " << std::flush;
    }
    std::cout << "\nbye.\n";
    return 0;
}
