#include "power_shout.h"

namespace {
    // Tier-scaled magnitudes: word 1 / word 2 / word 3.
    constexpr float kMagnitudes[3] = { 100.0f, 200.0f, 300.0f };

    // Duration in seconds. Recovery times are per-tier (higher tier = longer cooldown).
    constexpr std::uint32_t kDurationSeconds = 60;
    constexpr float         kRecoveryTimes[3] = { 20.0f, 45.0f, 90.0f };

    const char* kShoutName = "Power Enhancement";  // 力量強化
    const char* kWordTexts[3]         = { "Fus",  "Ro",  "Dah"  };   // placeholder dragon runes
    const char* kWordTranslations[3]  = { "Strength", "Power", "Surge" };

    RE::TESShout*       g_shout           = nullptr;
    RE::SpellItem*      g_spells[3]       = {};
    RE::TESWordOfPower* g_words[3]        = {};
    RE::EffectSetting*  g_effectHealth    = nullptr;
    RE::EffectSetting*  g_effectStamina   = nullptr;
    RE::EffectSetting*  g_effectMagicka   = nullptr;

    template <class T>
    T* CreateForm(const char* dbg) {
        auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<T>();
        if (!factory) {
            SKSE::log::error("[PowerShout] no IFormFactory for {}", dbg);
            return nullptr;
        }
        auto* raw = factory->Create();
        auto* typed = raw ? raw->template As<T>() : nullptr;
        if (!typed) {
            SKSE::log::error("[PowerShout] factory->Create() returned null for {}", dbg);
        } else {
            SKSE::log::trace("[PowerShout] created {} formID=0x{:08X}", dbg, typed->formID);
        }
        return typed;
    }

    RE::EffectSetting* MakeAvModEffect(RE::ActorValue av, const char* dbgName) {
        auto* eff = CreateForm<RE::EffectSetting>(dbgName);
        if (!eff) return nullptr;

        auto& d = eff->data;
        d.archetype      = RE::EffectArchetype::kValueModifier;
        d.primaryAV      = av;
        d.secondaryAV    = RE::ActorValue::kNone;
        d.resistVariable = RE::ActorValue::kNone;
        d.magicSkill     = RE::ActorValue::kNone;
        d.castingType    = RE::MagicSystem::CastingType::kFireAndForget;
        d.delivery       = RE::MagicSystem::Delivery::kSelf;
        d.baseCost       = 0.0f;
        d.taperWeight    = 0.0f;
        d.taperCurve     = 1.0f;
        d.taperDuration  = 0.0f;
        // kRecover: effect reverses when duration ends (so AV returns to base).
        // kNoHitEvent: don't fire hit events (it's a self buff).
        d.flags = RE::EffectSetting::EffectSettingData::Flag::kRecover |
                  RE::EffectSetting::EffectSettingData::Flag::kNoHitEvent;

        eff->fullName = dbgName;
        SKSE::log::info("[PowerShout] effect {} ready, AV={}, formID=0x{:08X}",
                        dbgName, static_cast<int>(av), eff->formID);
        return eff;
    }

    RE::Effect* MakeEffectEntry(RE::EffectSetting* base, float magnitude) {
        if (!base) return nullptr;
        auto* e = new RE::Effect();
        e->baseEffect = base;
        e->effectItem.magnitude = magnitude;
        e->effectItem.duration  = kDurationSeconds;
        e->effectItem.area      = 0;
        e->cost                 = 0.0f;
        e->conditions.head      = nullptr;
        return e;
    }

    RE::SpellItem* MakeTierSpell(int tier) {
        const auto dbg = std::format("PowerShout_Spell_Tier{}", tier + 1);
        auto* spell = CreateForm<RE::SpellItem>(dbg.c_str());
        if (!spell) return nullptr;

        spell->data.castingType   = RE::MagicSystem::CastingType::kFireAndForget;
        spell->data.delivery      = RE::MagicSystem::Delivery::kSelf;
        spell->data.spellType     = RE::MagicSystem::SpellType::kVoicePower;
        spell->data.chargeTime    = 0.0f;
        spell->data.costOverride  = 0;

        const float mag = kMagnitudes[tier];
        if (auto* e = MakeEffectEntry(g_effectHealth,  mag))  spell->effects.push_back(e);
        if (auto* e = MakeEffectEntry(g_effectStamina, mag))  spell->effects.push_back(e);
        if (auto* e = MakeEffectEntry(g_effectMagicka, mag))  spell->effects.push_back(e);

        spell->fullName = std::format("{} (tier {})", kShoutName, tier + 1);
        SKSE::log::info("[PowerShout] spell tier {} ready: +{:.0f} H/S/M for {}s, {} effects, formID=0x{:08X}",
                        tier + 1, mag, kDurationSeconds, spell->effects.size(), spell->formID);
        return spell;
    }

    RE::TESWordOfPower* MakeWord(int i) {
        const auto dbg = std::format("PowerShout_Word{}", i + 1);
        auto* w = CreateForm<RE::TESWordOfPower>(dbg.c_str());
        if (!w) return nullptr;
        w->fullName    = kWordTexts[i];
        w->translation = kWordTranslations[i];
        SKSE::log::info("[PowerShout] word {}/{} ready: '{}' = '{}', formID=0x{:08X}",
                        i + 1, 3, kWordTexts[i], kWordTranslations[i], w->formID);
        return w;
    }
}

void PowerShout::Initialize() {
    SKSE::log::info("[PowerShout] === Initialize begin ===");

    g_effectHealth  = MakeAvModEffect(RE::ActorValue::kHealth,  "PowerShout_Health");
    g_effectStamina = MakeAvModEffect(RE::ActorValue::kStamina, "PowerShout_Stamina");
    g_effectMagicka = MakeAvModEffect(RE::ActorValue::kMagicka, "PowerShout_Magicka");

    for (int i = 0; i < 3; ++i) {
        g_spells[i] = MakeTierSpell(i);
        g_words[i]  = MakeWord(i);
    }

    g_shout = CreateForm<RE::TESShout>("PowerShout_Shout");
    if (!g_shout) {
        SKSE::log::error("[PowerShout] === Initialize ABORTED: shout form null ===");
        return;
    }

    g_shout->fullName = kShoutName;

    for (int i = 0; i < 3; ++i) {
        g_shout->variations[i].word         = g_words[i];
        g_shout->variations[i].spell        = g_spells[i];
        g_shout->variations[i].recoveryTime = kRecoveryTimes[i];
        SKSE::log::debug("[PowerShout] variation[{}]: word={} spell={} recovery={}s",
                         i,
                         g_words[i]  ? std::format("0x{:08X}", g_words[i]->formID)  : std::string("null"),
                         g_spells[i] ? std::format("0x{:08X}", g_spells[i]->formID) : std::string("null"),
                         kRecoveryTimes[i]);
    }

    SKSE::log::info("[PowerShout] shout '{}' ready, formID=0x{:08X}", kShoutName, g_shout->formID);
    SKSE::log::info("[PowerShout] === Initialize end ===");
}

void PowerShout::EnsureGrantedToPlayer() {
    SKSE::log::info("[PowerShout] EnsureGrantedToPlayer begin");

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("[PowerShout] player singleton null, cannot grant shout");
        return;
    }
    if (!g_shout) {
        SKSE::log::warn("[PowerShout] shout form null (Initialize never ran or failed), skipping");
        return;
    }

    const bool hasAlready = player->HasShout(g_shout);
    SKSE::log::info("[PowerShout] player HasShout={}", hasAlready);

    if (!hasAlready) {
        player->AddShout(g_shout);
        SKSE::log::info("[PowerShout] AddShout called, player formID=0x{:08X}", player->formID);
    }

    // Try to unlock all three words so every tier is usable.
    // The vanilla "word wall" flow sets a "known" bit on the word-of-power form (0x40 in record flags).
    // Unclear whether just flipping the flag is enough without going through AbsorbShoutPoint() etc. —
    // we log both states so we can diagnose from the log.
    for (int i = 0; i < 3; ++i) {
        auto* w = g_words[i];
        if (!w) {
            SKSE::log::warn("[PowerShout] word[{}] null, cannot unlock", i);
            continue;
        }
        const std::uint32_t kKnownBit = 0x40;
        const bool before = (w->formFlags & kKnownBit) != 0;
        w->formFlags |= kKnownBit;
        const bool after = (w->formFlags & kKnownBit) != 0;
        SKSE::log::info("[PowerShout] word[{}] known flag: {} -> {}, formID=0x{:08X}",
                        i, before, after, w->formID);
    }

    SKSE::log::info("[PowerShout] EnsureGrantedToPlayer end");
}
