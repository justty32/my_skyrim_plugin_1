#include "FollowLight.h"
// SKSE::log comes from the precompiled header (SKSE/SKSE.h); do NOT include "log.h" here —
// it *defines* SetupLog(), so including it in a second TU causes a duplicate-symbol link error.
#include <atomic>
#include <chrono>
#include <SimpleIni.h>

// Engine-managed fill light: PlaceObjectAtMe a LIGH "bulb" ref and let the engine own it (render,
// cull, cross-cell, threading). We never touch ShadowSceneNode or the player skeleton — that races
// the engine's worker jobs and crashes (see PITFALLS.md #8). 4 brightness tiers live in
// ModForgeDaylight.esp (local FormIDs 0x800..0x803, Dynamic so they can follow the player).

namespace FollowLight
{
    struct Tier { const char* label; RE::FormID localId; };
    static constexpr Tier kTiers[] = {
        { "Off", 0x000 },
        { "Dim", 0x800 },
        { "Bright", 0x801 },
        { "Daylight", 0x802 },
        { "Blazing", 0x803 },
    };
    static constexpr std::size_t kTierCount = sizeof(kTiers) / sizeof(kTiers[0]);
    static constexpr const char* kEspName = "ModForgeDaylight.esp";

    // --- settings (ini-overridable) ---
    static std::uint32_t g_toggleKey = 0x26;   // DX scan code; 0x26 = 'L'
    static bool          g_follow = true;       // continuously reposition the bulb to the player
    static float         g_heightOffset = 120.0f;

    static std::size_t                           g_tier = 0;
    static RE::ObjectRefHandle                   g_handle{};
    static std::chrono::steady_clock::time_point g_lastPress{};
    static std::atomic<bool>                     g_applyPending{ false };

    static void LoadSettings()
    {
        CSimpleIniA ini;
        ini.SetUnicode();
        const char* path = "Data/SKSE/Plugins/DaylightDungeon/FollowLight.ini";
        if (ini.LoadFile(path) < 0) {
            SKSE::log::info("FollowLight: no ini at {}, using defaults", path);
            return;
        }
        g_toggleKey = static_cast<std::uint32_t>(ini.GetLongValue("Controls", "ToggleKey", 0x26));
        g_follow = ini.GetBoolValue("Light", "Follow", true);
        g_heightOffset = static_cast<float>(ini.GetDoubleValue("Light", "HeightOffset", 120.0));
        SKSE::log::info("FollowLight: ini key=0x{:X} follow={} height={}", g_toggleKey, g_follow, g_heightOffset);
    }

    static RE::TESObjectREFR* CurrentRef()
    {
        auto ref = g_handle.get();
        return ref ? ref.get() : nullptr;
    }

    static void DeleteCurrent()
    {
        if (auto* ref = CurrentRef()) {
            ref->Disable();
            ref->SetDelete(true);
        }
        g_handle = {};
    }

    static void Defer(std::function<void()> a_fn)
    {
        if (auto* task = SKSE::GetTaskInterface()) task->AddTask(std::move(a_fn));
    }

    // Per-frame follow: called from the PlayerCharacter::Update hook (hook.cpp) on the main thread.
    // Reposition the bulb to the player. NO task self-rescheduling — that dead-looped the main thread
    // (see PITFALLS.md #9). Cheap + guarded; a no-op when off / not following / no ref.
    void Update()
    {
        if (g_tier == 0 || !g_follow) return;
        auto* ref = CurrentRef();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!ref || !player) return;
        static bool s_logged = false;
        if (!s_logged) { s_logged = true; SKSE::log::info("FollowLight: Update hook firing — moving light (follow active)"); }
        const auto p = player->GetPosition();
        ref->SetPosition(p.x, p.y, p.z + g_heightOffset);
    }

    static void ApplyTierNow()
    {
        DeleteCurrent();
        if (g_tier == 0) return;

        auto* dh = RE::TESDataHandler::GetSingleton();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!dh || !player) return;

        auto* ligh = dh->LookupForm<RE::TESObjectLIGH>(kTiers[g_tier].localId, kEspName);
        if (!ligh) {
            SKSE::log::error("FollowLight: LIGH 0x{:X} not found in {} (esp installed + enabled?)",
                kTiers[g_tier].localId, kEspName);
            return;
        }
        auto ref = player->PlaceObjectAtMe(ligh, false);
        if (!ref) { SKSE::log::error("FollowLight: PlaceObjectAtMe failed"); return; }
        const auto pos = player->GetPosition();
        ref->SetPosition(pos.x, pos.y, pos.z + g_heightOffset);
        g_handle = ref->CreateRefHandle();
        SKSE::log::info("FollowLight: placed tier '{}' (LIGH 0x{:X})", kTiers[g_tier].label, kTiers[g_tier].localId);
    }

    static void RequestApply()
    {
        if (g_applyPending.exchange(true)) return;
        Defer([] {
            g_applyPending.store(false);
            ApplyTierNow();
        });
    }

    static void CycleTier()
    {
        g_tier = (g_tier + 1) % kTierCount;
        RequestApply();
        RE::DebugNotification((std::string("Daylight: ") + kTiers[g_tier].label).c_str());
    }

    // On a cell/map transition the placed temp ref belongs to the old cell (engine cleans it on
    // unload). Forget our handle + reset to off — no game calls during the transition window.
    static void OnCellChanged()
    {
        Defer([] {
            g_handle = {};
            g_tier = 0;
        });
    }

    class HotkeySink : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        static HotkeySink* GetSingleton() { static HotkeySink s; return &s; }
        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events,
            RE::BSTEventSource<RE::InputEvent*>*) override
        {
            if (!a_events) return RE::BSEventNotifyControl::kContinue;
            for (auto* e = *a_events; e; e = e->next) {
                auto* btn = e->AsButtonEvent();
                if (!btn || !btn->IsDown()) continue;
                if (btn->GetDevice() != RE::INPUT_DEVICE::kKeyboard) continue;
                if (btn->GetIDCode() != g_toggleKey) continue;
                const auto now = std::chrono::steady_clock::now();
                if (now - g_lastPress < 200ms) continue;  // debounce
                g_lastPress = now;
                CycleTier();
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    class CellSink : public RE::BSTEventSink<RE::BGSActorCellEvent>
    {
    public:
        static CellSink* GetSingleton() { static CellSink s; return &s; }
        RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event,
            RE::BSTEventSource<RE::BGSActorCellEvent>*) override
        {
            if (a_event && !a_event->flags.any(RE::BGSActorCellEvent::CellFlag::kLeave))
                OnCellChanged();
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void Initialize()
    {
        LoadSettings();
        if (auto* idm = RE::BSInputDeviceManager::GetSingleton()) {
            idm->AddEventSink(HotkeySink::GetSingleton());
            SKSE::log::info("FollowLight: hotkey sink registered (key 0x{:X})", g_toggleKey);
        } else {
            SKSE::log::error("FollowLight: BSInputDeviceManager null — hotkey not registered");
        }
        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            player->AsBGSActorCellEventSource()->AddEventSink(CellSink::GetSingleton());
            SKSE::log::info("FollowLight: player cell-change sink registered");
        }
    }
}
