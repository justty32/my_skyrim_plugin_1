#include "SkyrimAdapter.h"

#include <filesystem>
#include <fstream>

namespace skyrim {

namespace {

// CONFIG_FOLDER from CMakeLists.txt (currently "Template_Plugin"). Rename both
// together when this template becomes a real plugin (see CLAUDE.md).
constexpr const char* kConfigFolder = "Template_Plugin";

// Clock port (SPEC §5.7): game time in hours via RE::Calendar.
class CalendarClock : public qe::IClock {
public:
    double gameHours() override {
        if (auto* cal = RE::Calendar::GetSingleton()) {
            return static_cast<double>(cal->GetHoursPassed());
        }
        return 0.0;
    }
};

// Logger port: route the core's logs through SKSE::log.
class SkseLogger : public qe::ILogger {
public:
    void log(const std::string& msg) override { SKSE::log::info("[qe] {}", msg); }
};

// Run a callable on the main thread (RE:: state must only be touched there).
template <class F>
void OnMainThread(F&& f) {
    if (auto* task = SKSE::GetTaskInterface()) {
        task->AddTask(std::forward<F>(f));
    } else {
        // No task interface (very early / tests): run inline as a fallback.
        f();
    }
}

}  // namespace

SkyrimAdapter* SkyrimAdapter::GetSingleton() {
    static SkyrimAdapter singleton;
    return &singleton;
}

std::string SkyrimAdapter::ResolveQuestPath(const std::string& fileName) const {
    namespace fs = std::filesystem;

    // Primary: the config tree copied next to the DLL by the post-build step
    // (CMakeLists.txt) -> Data/SKSE/Plugins/<CONFIG_FOLDER>/quests/<file>.
    const fs::path primary =
        fs::path("Data") / "SKSE" / "Plugins" / kConfigFolder / "quests" / fileName;
    if (fs::exists(primary)) return primary.string();

    // Fallback: a Data-relative quests dir (in case the modder dropped the JSON
    // straight under Data/SKSE/Plugins/quests/).
    const fs::path fallback =
        fs::path("Data") / "SKSE" / "Plugins" / "quests" / fileName;
    if (fs::exists(fallback)) return fallback.string();

    // Last resort: return primary so the failure message names the expected path.
    return primary.string();
}

bool SkyrimAdapter::BuildEngine(nlohmann::json doc) {
    clock_ = std::make_unique<CalendarClock>();
    logger_ = std::make_unique<SkseLogger>();

    // SPEC §2.4: globals are declared up front at the system level. Phase 1 has
    // no co-save yet (DEFERRED), so we seed the cross-cycle counter the demo uses.
    if (globals_.vars.find("whiterun_tasks_done") == globals_.vars.end()) {
        globals_.vars["whiterun_tasks_done"] = 0.0;
    }

    // EntityResolver: pre-bind the quest's characters block (main thread).
    if (doc.contains("characters")) entities_.bindCharacters(doc["characters"]);

    // MessageBox callback -> resume the engine on the main thread.
    presenter_.setChoiceSink([](int idx) { SkyrimAdapter::GetSingleton()->SubmitChoice(idx); });

    qe::QuestEngine::Deps deps;
    deps.presenter = &presenter_;
    deps.clock = clock_.get();
    deps.logger = logger_.get();
    deps.actionRunner = &actions_;
    deps.condEval = &conditions_;
    deps.globals = &globals_;

    engine_ = std::make_unique<qe::QuestEngine>(std::move(doc), deps);

    // EventSource: install game sinks; sink marshals events onto the main thread.
    events_.install(
        [](const std::string& on, const nlohmann::json& filter) {
            SkyrimAdapter::GetSingleton()->FireEvent(on, filter);
        },
        entities_);

    // Temporary in-game testability hooks (DEFERRED removal): F10 fires
    // spell_cast_on{victim}, F9 polls timers (after the player waits/sleeps).
    events_.installDebugHotkeys([]() { SkyrimAdapter::GetSingleton()->CheckTimers(); });

    return true;
}

bool SkyrimAdapter::StartDemoQuest() {
    const std::string path = ResolveQuestPath("demo_court_wizard.json");
    std::ifstream in(path);
    if (!in) {
        SKSE::log::error("SkyrimAdapter: cannot open quest file: {}", path);
        return false;
    }
    nlohmann::json doc;
    try {
        in >> doc;
    } catch (const std::exception& e) {
        SKSE::log::error("SkyrimAdapter: JSON parse error in {}: {}", path, e.what());
        return false;
    }
    SKSE::log::info("SkyrimAdapter: loaded quest '{}' from {}",
                    doc.value("title", doc.value("id", "?")), path);

    if (!BuildEngine(std::move(doc))) return false;

    // Start the engine on the main thread (on_start may show a message / open a
    // dialogue immediately).
    OnMainThread([]() {
        auto* self = SkyrimAdapter::GetSingleton();
        if (self->engine_) {
            self->engine_->start();
            SKSE::log::info("SkyrimAdapter: engine started");
        }
    });
    return true;
}

void SkyrimAdapter::SubmitChoice(int idx) {
    OnMainThread([idx]() {
        auto* self = SkyrimAdapter::GetSingleton();
        if (self->engine_ && self->engine_->awaitingChoice()) {
            self->engine_->submitChoice(idx);
        } else {
            SKSE::log::warn("SkyrimAdapter: SubmitChoice({}) but engine not awaiting", idx);
        }
    });
}

void SkyrimAdapter::FireEvent(const std::string& on, const nlohmann::json& filter) {
    OnMainThread([on, filter]() {
        auto* self = SkyrimAdapter::GetSingleton();
        if (self->engine_) self->engine_->dispatchEvent(on, filter);
    });
}

void SkyrimAdapter::CheckTimers() {
    OnMainThread([]() {
        auto* self = SkyrimAdapter::GetSingleton();
        if (self->engine_) self->engine_->checkTimers();
    });
}

}  // namespace skyrim
