#pragma once

// Capability ports (SPEC §5): abstract interfaces the portable core calls to
// reach the host (game engine, or the CLI test harness). The core depends on
// nlohmann-json (allowed) but never on RE::/SKSE::.

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace qe {

// SPEC §5.5 — present one already-filtered dialogue node. DISPLAY-ONLY: this
// does NOT block for input (Skyrim's MessageBox is async; blocking the main
// thread would freeze the game). For a choice node (choices non-empty) the
// adapter feeds the player's pick back via QuestEngine::submitChoice() once the
// UI callback fires; for a terminal node (choices empty) the engine ends the
// dialogue immediately and no choice is expected.
class IDialoguePresenter {
public:
    virtual ~IDialoguePresenter() = default;
    virtual void presentNode(const std::string& speaker,
                             const std::vector<std::string>& lines,
                             const std::vector<std::string>& choices) = 0;
    virtual void showMessage(const std::string& text) = 0;
};

// SPEC §5.7 — game time in hours. Required once schedule/timer is used.
class IClock {
public:
    virtual ~IClock() = default;
    virtual double gameHours() = 0;
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string& msg) = 0;
};

// SPEC §5.3 — evaluate an adapter-extension condition. Unknown/unevaluable
// SHOULD return false rather than abort.
class IConditionEvaluator {
public:
    virtual ~IConditionEvaluator() = default;
    virtual bool evaluate(const std::string& key, const nlohmann::json& params) = 0;
};

// SPEC §5.2 — run an adapter-extension action.
class IActionRunner {
public:
    virtual ~IActionRunner() = default;
    virtual void run(const std::string& verb, const nlohmann::json& params) = 0;
};

}  // namespace qe
