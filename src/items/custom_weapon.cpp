#include "custom_weapon.h"

#include <cmath>

namespace {
    // Vanilla Iron Sword — template we copy model / equipSlot from.
    constexpr RE::FormID kTemplateWeaponFormID = 0x00012EB7;

    const char* kWeaponName = "Test Sword";   // 測試劍

    // Placement params when dropping in front of player.
    constexpr float kForwardDistance = 100.0f;  // game units; ~1.4m in Skyrim's scale
    constexpr float kDropHeight      = 60.0f;   // above player origin so it falls

    // Custom stats
    constexpr float         kAttackDamage = 25.0f;
    constexpr float         kWeight       = 8.0f;
    constexpr std::uint32_t kGoldValue    = 250;

    RE::TESObjectWEAP* g_weapon = nullptr;
}

void CustomWeapon::Initialize() {
    SKSE::log::info("[CustomWeapon] === Initialize begin ===");

    if (g_weapon) {
        SKSE::log::info("[CustomWeapon] already initialized, formID=0x{:08X}", g_weapon->formID);
        return;
    }

    auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectWEAP>();
    if (!factory) {
        SKSE::log::error("[CustomWeapon] no IFormFactory for TESObjectWEAP");
        return;
    }

    auto* form = factory->Create();
    g_weapon = form ? form->As<RE::TESObjectWEAP>() : nullptr;
    if (!g_weapon) {
        SKSE::log::error("[CustomWeapon] factory->Create() returned null");
        return;
    }
    SKSE::log::info("[CustomWeapon] created blank TESObjectWEAP formID=0x{:08X}", g_weapon->formID);

    auto* tmpl = RE::TESForm::LookupByID<RE::TESObjectWEAP>(kTemplateWeaponFormID);
    if (!tmpl) {
        SKSE::log::error("[CustomWeapon] template weapon 0x{:08X} not found — iron sword should always exist",
                         kTemplateWeaponFormID);
        return;
    }
    SKSE::log::info("[CustomWeapon] template '{}' found, copying model/slot", tmpl->GetName());

    // Copy model path (nif) and equip slot from template — we have no assets of our own.
    g_weapon->SetModel(tmpl->GetModel());
    g_weapon->equipSlot = tmpl->equipSlot;

    // Weapon classification: reuse template's weapon data as base so keywords / animation type / sound
    // match a 1H sword, then override the stats we care about.
    g_weapon->weaponData = tmpl->weaponData;
    g_weapon->criticalData = tmpl->criticalData;

    g_weapon->attackDamage = kAttackDamage;
    g_weapon->weight       = kWeight;
    g_weapon->value        = kGoldValue;
    g_weapon->fullName     = kWeaponName;

    SKSE::log::info("[CustomWeapon] weapon ready: name='{}' dmg={} wgt={} val={} formID=0x{:08X}",
                    kWeaponName, kAttackDamage, kWeight, kGoldValue, g_weapon->formID);
    SKSE::log::info("[CustomWeapon] === Initialize end ===");
}

void CustomWeapon::DropInFrontOfPlayer() {
    SKSE::log::info("[CustomWeapon] DropInFrontOfPlayer begin");

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("[CustomWeapon] player singleton null");
        return;
    }
    if (!g_weapon) {
        SKSE::log::warn("[CustomWeapon] weapon form null (Initialize never ran or failed)");
        return;
    }

    // Player state
    const auto  playerPos = player->GetPosition();
    const float yaw       = player->data.angle.z;   // radians, 0 = facing +Y (north)
    const float sinY      = std::sin(yaw);
    const float cosY      = std::cos(yaw);

    const RE::NiPoint3 dropPos{
        playerPos.x + sinY * kForwardDistance,
        playerPos.y + cosY * kForwardDistance,
        playerPos.z + kDropHeight,
    };

    SKSE::log::info("[CustomWeapon] player at ({:.1f},{:.1f},{:.1f}) yaw={:.3f}rad, drop at ({:.1f},{:.1f},{:.1f})",
                    playerPos.x, playerPos.y, playerPos.z, yaw,
                    dropPos.x, dropPos.y, dropPos.z);

    // PlaceObjectAtMe spawns a persistent ref at the caller's location.
    auto refPtr = player->PlaceObjectAtMe(g_weapon, true);
    if (!refPtr) {
        SKSE::log::error("[CustomWeapon] PlaceObjectAtMe returned null");
        return;
    }

    auto* ref = refPtr.get();
    if (!ref) {
        SKSE::log::error("[CustomWeapon] placed handle has null ref");
        return;
    }
    SKSE::log::info("[CustomWeapon] placed ref formID=0x{:08X}, relocating to drop point", ref->formID);

    ref->SetPosition(dropPos);

    SKSE::log::info("[CustomWeapon] DropInFrontOfPlayer end");
}
