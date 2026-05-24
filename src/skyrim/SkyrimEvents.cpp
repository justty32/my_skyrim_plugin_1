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

// Real `spell_cast_on` detection (research/spell_cast_on_hook.md): a magic effect
// was applied to a target. TESMagicEffectApplyEvent carries BOTH the caster and
// the target (TESSpellCastEvent has only the caster — that is why NpcGenerator has
// to read the crosshair separately), fires for player casts INCLUDING non-damaging
// effects (the demo's "cast a cure on the cursed retainer" case, which a hit event
// would miss), and needs no trampoline / RELOCATION_ID.
//
// We fire ONLY when the caster is the player AND the target maps to a tracked
// alias. Magic-effect applies are far more frequent than activations (weapon
// enchants, self-buffs, per-tick concentration effects), so — unlike ActivateSink
// — we do NOT emit a FormID fallback for untracked targets: an untracked target is
// silently ignored to avoid spurious fires and log spam. The engine's trigger
// filter still discriminates the specific {character}, so a non-victim alias is a
// harmless no-op. (Player ref is FormID 0x14; TESObjectREFR has no IsPlayerRef().)
class SkyrimEvents::MagicEffectSink : public RE::BSTEventSink<RE::TESMagicEffectApplyEvent> {
public:
    MagicEffectSink(EventSink* sink, SkyrimEntities* entities)
        : sink_(sink), entities_(entities) {}

    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESMagicEffectApplyEvent* a_event,
        RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) override {
        if (!a_event || !sink_ || !*sink_ || !entities_) return RE::BSEventNotifyControl::kContinue;
        auto* caster = a_event->caster.get();
        auto* target = a_event->target.get();
        if (!caster || !target) return RE::BSEventNotifyControl::kContinue;
        // Only the player casting ON someone else (skip self-applied effects).
        if (caster->GetFormID() != 0x14 || target->GetFormID() == 0x14) {
            return RE::BSEventNotifyControl::kContinue;
        }
        const std::string alias = entities_->aliasForFormID(target->GetFormID());
        if (alias.empty()) return RE::BSEventNotifyControl::kContinue;  // untracked target

        nlohmann::json filter;
        filter["character"] = alias;
        // Log the real fire — without this the sink is silent and a quest advance
        // leaves no trace in the log (this gap once made verification ambiguous).
        SKSE::log::info("SkyrimEvents: player cast on '{}' (target {:08X}, effect {:08X}) "
                        "-> spell_cast_on", alias, target->GetFormID(), a_event->magicEffect);
        (*sink_)("spell_cast_on", filter);
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

    // `spell_cast_on` detection (research/spell_cast_on_hook.md): a magic effect
    // applied to a target. Carries caster + target, fires for player casts incl.
    // non-damaging effects, zero RELOCATION_ID. Replaced the F10 debug fake, which
    // was removed after this sink was verified in-game (2026-05-24).
    if (!magicSink_) {
        magicSink_ = new MagicEffectSink(&sink_, &entities);
        holder->AddEventSink<RE::TESMagicEffectApplyEvent>(magicSink_);
        SKSE::log::info("SkyrimEvents: TESMagicEffectApplyEvent sink installed (spell_cast_on)");
    }
}

void SkyrimEvents::uninstall() {
    auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (holder && activateSink_) {
        holder->RemoveEventSink<RE::TESActivateEvent>(activateSink_);
    }
    if (holder && magicSink_) {
        holder->RemoveEventSink<RE::TESMagicEffectApplyEvent>(magicSink_);
    }
    delete activateSink_;
    activateSink_ = nullptr;
    delete magicSink_;
    magicSink_ = nullptr;
}

// Temporary debug input sink (DESIGN: "even a temporary debug hotkey"): lets a
// tester drive the quest loop in-game without waiting on real game state. F7
// force-fires scheduled timers, F8 polls due timers. (The old F10 spell_cast_on
// fake was removed once the real TESMagicEffectApplyEvent sink was verified
// in-game, 2026-05-24.)
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
                        "(F7=force-fire timers, F8=poll timers)");
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
