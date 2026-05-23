#include "SkyrimAdapter.h"

#include <filesystem>
#include <fstream>
#include <mutex>

#include <SKSE/API.h>
#include <SKSE/Interfaces.h>

#include "skyrim/CoSave.h"  // central co-save dispatcher (one SetUniqueID per plugin)

namespace skyrim {

namespace {

// CONFIG_FOLDER from CMakeLists.txt (currently "Template_Plugin"). Rename both
// together when this template becomes a real plugin (see CLAUDE.md).
constexpr const char* kConfigFolder = "Template_Plugin";

// ---- co-save staging (mirrors ProcgenNpc's pattern) ------------------------
// OnSave/OnLoad/OnRevert run on SKSE's serialization thread; RebuildStaged runs
// on the main thread. g_mutex guards the staged blob shared between them. The
// blob is the raw {progress, globals} JSON STRING captured at save; OnLoad reads
// it into g_stagedBlob and clears g_hasStaged=>true, RebuildStaged drains it.
std::mutex g_mutex;
std::string g_stagedBlob;  // raw JSON text staged by OnLoad for the main thread
bool g_hasStaged = false;  // a load happened (distinct from an empty blob)

// 1 MiB sanity cap on the length-prefixed string — a corrupt length must never
// drive an allocation that OOMs the game (same guard as ProcgenNpc::ReadString).
constexpr std::uint32_t kMaxBlobLen = 1u << 20;

bool WriteString(SKSE::SerializationInterface* intfc, const std::string& s) {
    const auto len = static_cast<std::uint32_t>(s.size());
    if (!intfc->WriteRecordData(len)) return false;
    if (len == 0) return true;
    return intfc->WriteRecordData(s.data(), len);
}

bool ReadString(SKSE::SerializationInterface* intfc, std::string& out) {
    std::uint32_t len = 0;
    if (intfc->ReadRecordData(len) == 0) return false;
    out.clear();
    if (len == 0) return true;
    if (len > kMaxBlobLen) {
        SKSE::log::error("SkyrimAdapter: refusing absurd co-save blob length {}", len);
        return false;
    }
    out.resize(len);
    return intfc->ReadRecordData(out.data(), len) == len;
}

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

bool SkyrimAdapter::LoadDemoQuest() {
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

    // Build the engine ONLY (no start()). on_start side effects (intro
    // show_message, scheduling) belong to a fresh new-game or to importProgress
    // on a save load — NOT to kDataLoaded (main-menu time). StartNewQuest() runs
    // start() on kNewGame; RebuildStaged() restores progress on kPostLoadGame.
    return BuildEngine(std::move(doc));
}

void SkyrimAdapter::StartNewQuest() {
    // New game: run on_start now (intro message + initial schedule). Marshalled
    // onto the main thread because on_start may show a message / open a dialogue.
    OnMainThread([]() {
        auto* self = SkyrimAdapter::GetSingleton();
        if (self->engine_) {
            self->engine_->start();
            SKSE::log::info("SkyrimAdapter: engine started (new game)");
        }
    });
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

// ---------------------------------------------------------------------------
// Co-save persistence (SPEC §6). The state is PURE JSON (no pointers, no game
// runtime ids — the engine keys everything by stable string ids per §6), so
// unlike ProcgenNpc there is nothing to ResolveFormID-remap: we just write one
// length-prefixed JSON string and read it back. Threading mirrors ProcgenNpc:
// Save/Load/Revert on the serialization thread (no RE:: live state touched),
// apply on the main thread from kPostLoadGame via RebuildStaged().
// ---------------------------------------------------------------------------

void SkyrimAdapter::OnSave(SKSE::SerializationInterface* intfc) {
    auto* self = SkyrimAdapter::GetSingleton();
    // Reading the engine's in-memory state here is a const snapshot of plain
    // std containers; it does not touch RE:: live state, so it is safe off the
    // main thread. (Mutators all run via OnMainThread, so the engine is not being
    // structurally rebuilt concurrently during the SKSE save callback.)
    nlohmann::json blob = nlohmann::json::object();
    blob["progress"] = self->engine_ ? self->engine_->exportProgress()
                                      : nlohmann::json::object();
    blob["globals"] = qe::serializeGlobals(self->globals_);
    const std::string text = blob.dump();

    if (!intfc->OpenRecord(kRecordType, kRecordVersion)) {
        SKSE::log::error("SkyrimAdapter: OnSave OpenRecord('QEST') failed");
        return;
    }
    if (!WriteString(intfc, text)) {
        SKSE::log::error("SkyrimAdapter: OnSave failed to write progress blob");
        return;
    }
    SKSE::log::info("SkyrimAdapter: OnSave wrote {}-byte 'QEST' progress blob",
                    text.size());
}

void SkyrimAdapter::OnLoad(SKSE::SerializationInterface* intfc, std::uint32_t version,
                           std::uint32_t /*length*/) {
    // Serialization thread: read ONLY this 'QEST' record's payload (the dispatcher
    // already consumed the header) and STAGE it. RebuildStaged() applies it on the
    // next kPostLoadGame main-thread tick (engine already built at kDataLoaded).
    std::scoped_lock lock(g_mutex);
    g_stagedBlob.clear();
    g_hasStaged = false;

    if (version != kRecordVersion) {
        SKSE::log::warn("SkyrimAdapter: OnLoad 'QEST' version {} != {} (ignored)",
                        version, kRecordVersion);
        return;
    }
    std::string text;
    if (!ReadString(intfc, text)) {
        SKSE::log::error("SkyrimAdapter: OnLoad failed to read 'QEST' progress blob");
        return;
    }
    g_stagedBlob = std::move(text);
    g_hasStaged = true;
    SKSE::log::info("SkyrimAdapter: OnLoad staged {}-byte 'QEST' blob for main-thread apply",
                    g_stagedBlob.size());
}

void SkyrimAdapter::OnRevert(SKSE::SerializationInterface*) {
    // Revert: drop any staged data and reset in-memory state ONLY (mirrors the
    // procgen handlers' OnRevert, which just clear their registry). This runs on
    // the serialization thread, so it must NOT touch RE:: live state nor re-run
    // the quest: start() -> on_start would call show_message -> RE::DebugNotification
    // off the main thread (UB/crash). resetState() clears the engine's std state
    // WITHOUT running on_start; the real (re)start happens at kDataLoaded (build)
    // / kNewGame (start) / kPostLoadGame (importProgress) on the main thread.
    {
        std::scoped_lock lock(g_mutex);
        g_stagedBlob.clear();
        g_hasStaged = false;
    }
    auto* self = SkyrimAdapter::GetSingleton();
    self->globals_.vars.clear();
    if (self->engine_) self->engine_->resetState();
    SKSE::log::info("SkyrimAdapter: OnRevert reset engine + globals (no side effects)");
}

bool SkyrimAdapter::Register() {
    // Register a 'QEST' handler with the central dispatcher (CoSave.h: one
    // SetUniqueID per plugin — doing it here would clobber the 'GNPC'/'PRGN'
    // handlers). Mirrors ProcgenNpc::Register.
    skyrim::cosave::AddHandler({ kRecordType, &SkyrimAdapter::OnSave,
                                 &SkyrimAdapter::OnLoad, &SkyrimAdapter::OnRevert });
    SKSE::log::info("SkyrimAdapter: registered 'QEST' co-save handler with dispatcher");
    return true;
}

void SkyrimAdapter::RebuildStaged() {
    // Drain the staged blob under the lock, then apply on the main thread. If
    // nothing was staged (brand new game, or no 'QEST' record) this is a no-op:
    // a new game runs on_start via StartNewQuest() at kNewGame instead.
    std::string blobText;
    bool had = false;
    {
        std::scoped_lock lock(g_mutex);
        if (g_hasStaged) {
            blobText.swap(g_stagedBlob);
            had = true;
            g_hasStaged = false;
        }
    }
    if (!had) return;

    OnMainThread([blobText = std::move(blobText)]() {
        auto* self = SkyrimAdapter::GetSingleton();
        nlohmann::json blob;
        try {
            blob = nlohmann::json::parse(blobText);
        } catch (const std::exception& e) {
            SKSE::log::error("SkyrimAdapter: RebuildStaged bad 'QEST' JSON: {}", e.what());
            return;
        }
        // Restore system globals first (declared by BuildEngine), then layer the
        // quest progress onto the already-built engine (SPEC §6 "定義與進度分離":
        // the definition was re-loaded at kDataLoaded; we apply progress on top).
        qe::restoreGlobals(self->globals_, blob.value("globals", nlohmann::json::object()));
        if (self->engine_) {
            self->engine_->importProgress(blob.value("progress", nlohmann::json::object()));
            SKSE::log::info("SkyrimAdapter: RebuildStaged applied 'QEST' progress + globals");
        } else {
            SKSE::log::warn("SkyrimAdapter: RebuildStaged has no engine to apply 'QEST' to");
        }
    });
}

}  // namespace skyrim
