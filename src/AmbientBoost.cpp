#include "AmbientBoost.h"
// SKSE::log via PCH (do not include log.h — it defines SetupLog()).
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <SimpleIni.h>

namespace AmbientBoost
{
    using Inherit = RE::INTERIOR_DATA::Inherit;

    // Per-level ambient floor (0..255). Warm tint (G/B scaled down vs R) applied below.
    struct Level { const char* label; std::uint8_t floor; };
    static constexpr Level kLevels[] = {
        { "Off", 0 },
        { "Ambient Low", 60 },
        { "Ambient Med", 110 },
        { "Ambient High", 170 },
        { "Ambient Max", 220 },
    };
    static constexpr std::size_t kLevelCount = sizeof(kLevels) / sizeof(kLevels[0]);
    static constexpr float kWarmG = 0.93f, kWarmB = 0.82f;

    // Saved original lighting per cell so Off restores exactly.
    struct Saved
    {
        RE::Color                            ambient, directional;
        RE::Color                            dalc[6];
        REX::EnumSet<Inherit, std::uint32_t> flags;
    };
    static std::unordered_map<RE::FormID, Saved> g_saved;

    static std::uint32_t                         g_key = 0x25;  // 'K'
    static std::size_t                           g_level = 0;
    static std::chrono::steady_clock::time_point g_lastPress{};

    static void LoadSettings()
    {
        CSimpleIniA ini; ini.SetUnicode();
        if (ini.LoadFile("Data/SKSE/Plugins/Template_Plugin/FollowLight.ini") < 0) return;
        g_key = static_cast<std::uint32_t>(ini.GetLongValue("Ambient", "ToggleKey", 0x25));
        SKSE::log::info("AmbientBoost: ini key=0x{:X}", g_key);
    }

    // Collect the 6 directional-ambient (DALC) colour slots of a cell's interior data.
    static void DalcSlots(RE::INTERIOR_DATA* d, RE::Color* out[6])
    {
        auto& g = d->directionalAmbientLightingColors.directional;
        out[0] = &g.x.max; out[1] = &g.x.min;
        out[2] = &g.y.max; out[3] = &g.y.min;
        out[4] = &g.z.max; out[5] = &g.z.min;
    }

    static inline void Lift(std::uint8_t& c, std::uint8_t floor) { c = std::max(c, floor); }
    static void LiftColor(RE::Color& c, std::uint8_t f)
    {
        Lift(c.red, f);
        Lift(c.green, static_cast<std::uint8_t>(f * kWarmG));
        Lift(c.blue, static_cast<std::uint8_t>(f * kWarmB));
    }

    static void ApplyToCurrentCell()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* cell = player ? player->GetParentCell() : nullptr;
        if (!cell || !cell->IsInteriorCell()) {
            RE::DebugNotification("Ambient: indoors only");
            return;
        }
        auto* d = cell->GetLighting();
        if (!d) { SKSE::log::warn("AmbientBoost: cell has no interior lighting"); return; }

        const auto id = cell->GetFormID();
        RE::Color* dalc[6]; DalcSlots(d, dalc);

        // First touch of this cell: remember its originals.
        if (!g_saved.contains(id)) {
            Saved s;
            s.ambient = d->ambient;
            s.directional = d->directional;
            for (int i = 0; i < 6; ++i) s.dalc[i] = *dalc[i];
            s.flags = d->lightingTemplateInheritanceFlags;
            g_saved[id] = s;
        }

        // Always restore-from-original first so levels don't stack.
        const Saved& s = g_saved[id];
        d->ambient = s.ambient;
        d->directional = s.directional;
        for (int i = 0; i < 6; ++i) *dalc[i] = s.dalc[i];
        d->lightingTemplateInheritanceFlags = s.flags;

        if (g_level == 0) {
            RE::DebugNotification("Ambient: Off (re-enter cell to apply)");
            return;
        }

        // Stop inheriting ambient/directional from the lighting template so our cell values win.
        d->lightingTemplateInheritanceFlags.reset(Inherit::kAmbientColor, Inherit::kDirectionalColor);
        const std::uint8_t f = kLevels[g_level].floor;
        LiftColor(d->ambient, f);
        LiftColor(d->directional, f);
        for (int i = 0; i < 6; ++i) LiftColor(*dalc[i], f);

        RE::DebugNotification((std::string(kLevels[g_level].label) + " — re-enter cell to apply").c_str());
        SKSE::log::info("AmbientBoost: cell 0x{:X} level '{}' (floor={})", id, kLevels[g_level].label, f);
    }

    static void CycleLevel()
    {
        g_level = (g_level + 1) % kLevelCount;
        ApplyToCurrentCell();
    }

    void RestoreAll()
    {
        for (auto& [id, s] : g_saved) {
            auto* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(id);
            auto* d = cell ? cell->GetLighting() : nullptr;
            if (!d) continue;
            d->ambient = s.ambient;
            d->directional = s.directional;
            RE::Color* dalc[6]; DalcSlots(d, dalc);
            for (int i = 0; i < 6; ++i) *dalc[i] = s.dalc[i];
            d->lightingTemplateInheritanceFlags = s.flags;
        }
        g_saved.clear();
        g_level = 0;
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
                if (btn->GetIDCode() != g_key) continue;
                const auto now = std::chrono::steady_clock::now();
                if (now - g_lastPress < 200ms) continue;
                g_lastPress = now;
                CycleLevel();
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void Initialize()
    {
        LoadSettings();
        if (auto* idm = RE::BSInputDeviceManager::GetSingleton()) {
            idm->AddEventSink(HotkeySink::GetSingleton());
            SKSE::log::info("AmbientBoost: hotkey sink registered (key 0x{:X})", g_key);
        }
    }
}
