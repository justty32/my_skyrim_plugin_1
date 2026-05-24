#include "SkyrimEvents.h"

namespace skyrim {

// Low-risk game event: an actor/object was activated (talk/interact entry,
// DESIGN §2.4 `activate`). We reverse-map the activated ref to its bound alias
// and fire "activate" {character:<alias>} via the sink (which marshals onto the
// main thread inside the adapter). If the ref isn't a bound character we still
// fire with the FormID as a hex string so debug triggers can match.
class SkyrimEvents::ActivateSink : public RE::BSTEventSink<RE::TESActivateEvent> {
public:
    ActivateSink(EventSink* sink, SkyrimEntities* entities)
        : sink_(sink), entities_(entities) {}

    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESActivateEvent* a_event,
        RE::BSTEventSource<RE::TESActivateEvent>*) override {
        if (!a_event || !sink_ || !*sink_) return RE::BSEventNotifyControl::kContinue;
        auto* obj = a_event->objectActivated.get();
        if (!obj) return RE::BSEventNotifyControl::kContinue;

        std::string alias = entities_ ? entities_->aliasForFormID(obj->GetFormID()) : "";
        if (alias.empty()) {
            // Not a tracked character — emit the FormID so manual/debug triggers
            // can still match; keeps the activate path testable.
            char buf[16];
            std::snprintf(buf, sizeof(buf), "0x%08X", obj->GetFormID());
            alias = buf;
        }
        nlohmann::json filter;
        filter["character"] = alias;
        (*sink_)("activate", filter);
        return RE::BSEventNotifyControl::kContinue;
    }

private:
    EventSink* sink_;
    SkyrimEntities* entities_;
};

void SkyrimEvents::install(EventSink sink, SkyrimEntities& entities) {
    sink_ = std::move(sink);

    auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (!holder) {
        SKSE::log::error("SkyrimEvents: no ScriptEventSourceHolder");
        return;
    }
    if (!activateSink_) {
        activateSink_ = new ActivateSink(&sink_, &entities);
        holder->AddEventSink<RE::TESActivateEvent>(activateSink_);
        SKSE::log::info("SkyrimEvents: TESActivateEvent sink installed");
    }

    // TODO (open issue, progress.md / DESIGN §6): `spell_cast_on` detection.
    // TESSpellCastEvent (already used by NpcGenerator) reports only the CASTER
    // and spell, not the target the spell lands on, so it cannot by itself say
    // "a spell was cast ON <victim>". Candidate approaches to evaluate later:
    //   1. A magic-effect-applied hook (Actor::MagicTarget / ActiveEffect::OnAdd)
    //      filtered to the curse effect, reading the effect's target actor.
    //   2. On the caster's TESSpellCastEvent, read the player's crosshair/target
    //      ref (RE::CrosshairPickData, as NpcGenerator does) as the "victim".
    //   3. A trampoline hook on the spell-delivery / hit routine.
    // All are R&D, not a proven recipe — NOT wired here to avoid a risky hook.
    // For now the demo fires it manually via fireManual()/the debug hotkey.
}

void SkyrimEvents::uninstall() {
    auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (holder && activateSink_) {
        holder->RemoveEventSink<RE::TESActivateEvent>(activateSink_);
    }
    delete activateSink_;
    activateSink_ = nullptr;
}

// Temporary debug input sink (DESIGN: "even a temporary debug hotkey"): lets a
// tester drive the loop in-game before real detection hooks exist. REMOVE once
// spell_cast_on / world triggers are wired.
class SkyrimEvents::DebugInputSink : public RE::BSTEventSink<RE::InputEvent*> {
public:
    DebugInputSink(SkyrimEvents* owner, std::function<void()> onTimers,
                   std::function<void()> onForceFire)
        : owner_(owner), onTimers_(std::move(onTimers)),
          onForceFire_(std::move(onForceFire)) {}

    RE::BSEventNotifyControl ProcessEvent(
        RE::InputEvent* const* a_events,
        RE::BSTEventSource<RE::InputEvent*>*) override {
        if (!a_events || !owner_) return RE::BSEventNotifyControl::kContinue;
        for (auto* e = *a_events; e; e = e->next) {
            auto* btn = e->AsButtonEvent();
            if (!btn || !btn->IsDown()) continue;
            switch (btn->GetIDCode()) {
                case 0x41:  // F7 — force-fire all scheduled timers now (test the
                            // 48h summon without sleeping; starts the quest first)
                    if (onForceFire_) onForceFire_();
                    break;
                case 0x42:  // F8 — poll DUE timers (F9 conflicts with Quick Load)
                    if (onTimers_) onTimers_();
                    break;
                case 0x44: {  // F10
                    nlohmann::json f;
                    f["character"] = "victim";
                    owner_->fireManual("spell_cast_on", f);
                    break;
                }
                default:
                    break;
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

private:
    SkyrimEvents* owner_;
    std::function<void()> onTimers_;
    std::function<void()> onForceFire_;
};

void SkyrimEvents::installDebugHotkeys(std::function<void()> onTimers,
                                       std::function<void()> onForceFire) {
    auto* idm = RE::BSInputDeviceManager::GetSingleton();
    if (!idm) {
        SKSE::log::error("SkyrimEvents: no BSInputDeviceManager for debug hotkeys");
        return;
    }
    if (!debugSink_) {
        debugSink_ = new DebugInputSink(this, std::move(onTimers), std::move(onForceFire));
        idm->AddEventSink(debugSink_);
        SKSE::log::info("SkyrimEvents: debug hotkeys installed "
                        "(F7=force-fire timers, F8=poll timers, F10=spell_cast_on)");
    }
}

void SkyrimEvents::uninstallDebugHotkeys() {
    auto* idm = RE::BSInputDeviceManager::GetSingleton();
    if (idm && debugSink_) idm->RemoveEventSink(debugSink_);
    delete debugSink_;
    debugSink_ = nullptr;
}

void SkyrimEvents::fireManual(const std::string& on, const nlohmann::json& filter) {
    if (sink_) {
        SKSE::log::info("SkyrimEvents: fireManual '{}' {}", on, filter.dump());
        sink_(on, filter);
    } else {
        SKSE::log::warn("SkyrimEvents: fireManual '{}' but no sink installed", on);
    }
}

}  // namespace skyrim
