#pragma once

// SkyrimEvents (DESIGN §2.4) — EventSource port (SPEC §5.4). Subscribes to game
// event sources and forwards (eventType, filter-json) back into the engine so
// triggers fire (SPEC §3.3).
//
// The portable engine is single-threaded and must be driven on the main thread,
// so this class never touches the engine directly: it calls a sink callback
// (set by the adapter) which marshals onto the main thread via AddTask.
//
// Wired (low-risk): TESActivateEvent -> "activate" {character}, and
// TESMagicEffectApplyEvent -> "spell_cast_on" {character} (research/
// spell_cast_on_hook.md: that event carries BOTH the caster AND the target —
// unlike TESSpellCastEvent which has only the caster — fires for player casts
// including non-damaging effects, and needs zero RELOCATION_ID). The F10 debug
// hotkey is kept as a test fallback until the real sink is verified in-game.

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

#include "SkyrimEntities.h"

namespace skyrim {

// Adapter-supplied sink: (eventType, filter) -> engine.dispatchEvent on main thread.
using EventSink = std::function<void(const std::string&, const nlohmann::json&)>;

class SkyrimEvents {
public:
    // Register the sink and add the low-risk game event sinks. `entities` lets
    // the sinks reverse-map a game ref back to its JSON alias. Must be called on
    // the main thread after kDataLoaded (MODDING_COOKBOOK: AddEventSink timing).
    void install(EventSink sink, SkyrimEntities& entities);
    void uninstall();

    // Manually inject an event into the engine — testability hook for triggers
    // whose real game detection isn't wired yet (e.g. spell_cast_on). Routed via
    // the same sink, so it lands on the main thread.
    void fireManual(const std::string& on, const nlohmann::json& filter);

    EventSink& sink() { return sink_; }

    // Install a temporary debug hotkey sink so the whole loop is testable in-game
    // before real detection (e.g. spell_cast_on) is wired. Keys (DX scancodes):
    //   F7  -> force-fire ALL scheduled timers now (start the quest first if
    //          needed) — drives the demo's after_hours:48 summon without waiting
    //   F8  -> CheckTimers poll       (advance DUE timers after waiting in-game)
    //   F10 -> fireManual("spell_cast_on", {"character":"victim"})
    // (F9 is intentionally avoided — it is Skyrim's Quick Load.)
    // `onTimers` is called for F8 (routed to QuestEngine::checkTimers); `onForceFire`
    // is called for F7 (routed to SkyrimAdapter::DebugForceFireTimers).
    void installDebugHotkeys(std::function<void()> onTimers,
                             std::function<void()> onForceFire);
    void uninstallDebugHotkeys();

private:
    EventSink sink_;
    class ActivateSink;       // defined in the .cpp
    class MagicEffectSink;    // defined in the .cpp
    class DebugInputSink;     // defined in the .cpp
    ActivateSink* activateSink_ = nullptr;
    MagicEffectSink* magicSink_ = nullptr;
    DebugInputSink* debugSink_ = nullptr;
};

}  // namespace skyrim
