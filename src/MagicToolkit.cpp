#include "MagicToolkit.h"
#include "RE/C/CrosshairPickData.h"
#include "RE/P/Projectile.h"

namespace MagicToolkit
{
    // ──────────────────────────────────────────────────────────────────────────
    // Spell registry
    // ──────────────────────────────────────────────────────────────────────────

    RE::SpellItem* g_spawnCitizenSpell  = nullptr;
    RE::SpellItem* g_tameNpcSpell       = nullptr;
    RE::SpellItem* g_cycleScaleSpell    = nullptr;
    RE::SpellItem* g_spawnTreeSpell     = nullptr;
    RE::SpellItem* g_spawnRockSpell     = nullptr;
    RE::SpellItem* g_raiseObjectSpell   = nullptr;
    RE::SpellItem* g_lowerObjectSpell   = nullptr;
    RE::SpellItem* g_meteorRainSpell    = nullptr;
    RE::SpellItem* g_explosionSpell     = nullptr;
    RE::SpellItem* g_restoreSpell       = nullptr;
    RE::SpellItem* g_addGoldSpell       = nullptr;
    RE::SpellItem* g_giveWeaponSpell    = nullptr;
    RE::SpellItem* g_inspectSpell       = nullptr;
    // Wave 2
    RE::SpellItem* g_timeFreezeSpell    = nullptr;
    RE::SpellItem* g_spawnGuardSpell    = nullptr;
    RE::SpellItem* g_teleportSpell      = nullptr;
    RE::SpellItem* g_healTargetSpell    = nullptr;
    // Wave 3
    RE::SpellItem* g_forcePushSpell     = nullptr;
    RE::SpellItem* g_godModeSpell       = nullptr;
    RE::SpellItem* g_randomTeleSpell    = nullptr;
    RE::SpellItem* g_cloneObjectSpell   = nullptr;
    // Wave 4
    RE::SpellItem* g_levelUpSpell       = nullptr;
    RE::SpellItem* g_deleteTargetSpell  = nullptr;
    RE::SpellItem* g_paralyzeSpell      = nullptr;
    RE::SpellItem* g_spawnDragonSpell   = nullptr;
    // Wave 5
    RE::SpellItem* g_weatherSpell       = nullptr;
    RE::SpellItem* g_frenzySpell        = nullptr;
    RE::SpellItem* g_lootSpell          = nullptr;
    RE::SpellItem* g_speedSpell         = nullptr;

    // ──────────────────────────────────────────────────────────────────────────
    // Helpers
    // ──────────────────────────────────────────────────────────────────────────

    static RE::TESObjectREFR* GetCrosshairRef()
    {
        auto* pick = RE::CrosshairPickData::GetSingleton();
        if (!pick) return nullptr;
        auto handle = pick->target;
        auto ref    = handle.get();
        return ref.get();
    }

    // Spawn a bound object 200 units in front of the caster.
    static RE::TESObjectREFR* SpawnInFront(RE::TESBoundObject* a_base, RE::Actor* a_caster, float a_dist = 200.0f)
    {
        if (!a_base || !a_caster) return nullptr;

        float angleZ = a_caster->data.angle.z;
        RE::NiPoint3 forward{ std::sin(angleZ), std::cos(angleZ), 0.0f };
        RE::NiPoint3 pos = a_caster->GetPosition() + forward * a_dist;

        auto* ref = a_caster->PlaceObjectAtMe(a_base, false);
        if (ref) {
            ref->SetPosition(pos);
            ref->data.angle = a_caster->data.angle;
        }
        return ref;
    }

    // ──────────────────────────────────────────────────────────────────────────
    // NPC features
    // ──────────────────────────────────────────────────────────────────────────

    void SpawnCitizen(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnCitizen: entry, caster={}", a_caster ? a_caster->GetName() : "null");
        if (!a_caster) return;

        auto* templateNPC = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);
        auto* factory     = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
        if (!templateNPC || !factory) {
            SKSE::log::error("SpawnCitizen: missing template or factory");
            return;
        }

        auto* npcBase = factory->Create()->As<RE::TESNPC>();
        if (!npcBase) {
            SKSE::log::error("SpawnCitizen: factory->Create() returned null");
            return;
        }
        npcBase->Copy(templateNPC);
        npcBase->fullName = "Generated Citizen";

        auto* ref = SpawnInFront(npcBase, a_caster, 150.0f);
        if (ref) {
            if (auto* actor = ref->As<RE::Actor>()) {
                actor->EvaluatePackage(false, true);
            }
            SKSE::log::info("SpawnCitizen: spawned at ({:.1f},{:.1f},{:.1f})", ref->GetPosition().x, ref->GetPosition().y, ref->GetPosition().z);
            RE::DebugNotification("A citizen has been summoned.");
        } else {
            SKSE::log::warn("SpawnCitizen: PlaceObjectAtMe returned null");
        }
    }

    void TameNPC(RE::Actor* a_target)
    {
        SKSE::log::info("TameNPC: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || a_target->IsPlayerRef()) return;

        a_target->SetActorValue(RE::ActorValue::kAggression, 0.0f);
        a_target->SetActorValue(RE::ActorValue::kConfidence, 0.0f);
        a_target->StopCombat();
        a_target->NotifyAnimationGraph("BleedoutStart");
        a_target->EvaluatePackage(false, true);

        SKSE::log::info("TameNPC: {} tamed", a_target->GetName());
        RE::DebugNotification(std::format("{} has been tamed.", a_target->GetName()).c_str());
    }

    void CycleNPCScale(RE::Actor* a_target)
    {
        SKSE::log::info("CycleNPCScale: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || a_target->IsPlayerRef()) return;

        static const float kScales[] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f };
        static const int   kCount    = static_cast<int>(std::size(kScales));

        float cur     = a_target->GetScale();
        int   nextIdx = 0;
        for (int i = 0; i < kCount; ++i) {
            if (std::abs(cur - kScales[i]) < 0.05f) {
                nextIdx = (i + 1) % kCount;
                break;
            }
        }

        float next = kScales[nextIdx];
        // refScale stores scale * 100 as uint16_t
        a_target->GetReferenceRuntimeData().refScale = static_cast<std::uint16_t>(next * 100.0f);
        a_target->DoReset3D(true);

        SKSE::log::info("CycleNPCScale: {} scale {:.2f} -> {:.2f}", a_target->GetName(), cur, next);
        RE::DebugNotification(std::format("{} scale: {:.1f}x", a_target->GetName(), next).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // World features
    // ──────────────────────────────────────────────────────────────────────────

    void SpawnTree(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnTree: entry");
        if (!a_caster) return;

        auto* treeBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("TreeFloraJuniper01");
        if (!treeBase) treeBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("TreePineForest01");
        if (!treeBase) treeBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00038432);

        if (!treeBase) {
            SKSE::log::error("SpawnTree: no tree base found");
            RE::DebugNotification("SpawnTree: tree form not found.");
            return;
        }

        auto* ref = SpawnInFront(treeBase, a_caster);
        if (ref) {
            SKSE::log::info("SpawnTree: spawned '{}' at ({:.1f},{:.1f},{:.1f})", treeBase->GetName(), ref->GetPosition().x, ref->GetPosition().y, ref->GetPosition().z);
            RE::DebugNotification("A tree has been planted.");
        }
    }

    void SpawnRock(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnRock: entry");
        if (!a_caster) return;

        auto* rockBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("RockPileM01");
        if (!rockBase) rockBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("RockCliff01");
        if (!rockBase) rockBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0001B983);

        if (!rockBase) {
            SKSE::log::error("SpawnRock: no rock base found");
            RE::DebugNotification("SpawnRock: rock form not found.");
            return;
        }

        auto* ref = SpawnInFront(rockBase, a_caster);
        if (ref) {
            SKSE::log::info("SpawnRock: spawned '{}' at ({:.1f},{:.1f},{:.1f})", rockBase->GetName(), ref->GetPosition().x, ref->GetPosition().y, ref->GetPosition().z);
            RE::DebugNotification("A rock has been summoned.");
        }
    }

    void RaiseObject(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("RaiseObject: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target) return;

        auto pos = a_target->GetPosition();
        pos.z += 100.0f;
        a_target->SetPosition(pos);

        SKSE::log::info("RaiseObject: '{}' raised to z={:.1f}", a_target->GetName(), pos.z);
        RE::DebugNotification(std::format("{} raised.", a_target->GetName()).c_str());
    }

    void LowerObject(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("LowerObject: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target) return;

        auto pos = a_target->GetPosition();
        pos.z -= 100.0f;
        a_target->SetPosition(pos);

        SKSE::log::info("LowerObject: '{}' lowered to z={:.1f}", a_target->GetName(), pos.z);
        RE::DebugNotification(std::format("{} lowered.", a_target->GetName()).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Combat / Magic features
    // ──────────────────────────────────────────────────────────────────────────

    static void LaunchFireball(RE::Actor* a_shooter, RE::NiPoint3 a_from, RE::NiPoint3 a_to)
    {
        // FireballProjectile (Vanilla Fireball spell projectile base)
        auto* projBase = RE::TESForm::LookupByEditorID<RE::BGSProjectile>("MagicFireball01Projectile");
        if (!projBase) projBase = RE::TESForm::LookupByID<RE::BGSProjectile>(0x00015CFD);
        if (!projBase) {
            SKSE::log::warn("LaunchFireball: projectile base not found");
            return;
        }

        RE::NiPoint3 dir = a_to - a_from;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len < 0.001f) return;
        dir /= len;

        float pitch = -std::asin(dir.z);
        float yaw   = std::atan2(dir.x, dir.y);

        RE::Projectile::LaunchData ld;
        ld.origin        = a_from;
        ld.projectileBase = projBase;
        ld.shooter       = a_shooter;
        ld.angleX        = pitch;
        ld.angleZ        = yaw;
        ld.parentCell    = a_shooter->GetParentCell();
        ld.spell         = nullptr;
        ld.castingSource = RE::MagicSystem::CastingSource::kRightHand;
        ld.power         = 1.0f;
        ld.scale         = 1.0f;
        ld.useOrigin     = true;
        ld.autoAim       = false;

        RE::ProjectileHandle result;
        RE::Projectile::Launch(&result, ld);
    }

    void MeteorRain(RE::Actor* a_caster)
    {
        SKSE::log::info("MeteorRain: entry");
        if (!a_caster) return;

        RE::NiPoint3 center = a_caster->GetPosition();

        // Seed once so each call is different
        static bool seeded = false;
        if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr))); seeded = true; }

        constexpr int kCount = 15;
        for (int i = 0; i < kCount; ++i) {
            RE::NiPoint3 skyPos = center;
            skyPos.x += static_cast<float>((std::rand() % 1000) - 500);
            skyPos.y += static_cast<float>((std::rand() % 1000) - 500);
            skyPos.z += 2000.0f;

            RE::NiPoint3 landPos = center;
            landPos.x += static_cast<float>((std::rand() % 1500) - 750);
            landPos.y += static_cast<float>((std::rand() % 1500) - 750);

            LaunchFireball(a_caster, skyPos, landPos);
        }

        SKSE::log::info("MeteorRain: launched {} fireballs", kCount);
        RE::DebugNotification("Meteor rain unleashed!");
    }

    void ExplosionBlast(RE::TESObjectREFR* a_target, RE::Actor* a_caster)
    {
        SKSE::log::info("ExplosionBlast: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_caster) return;

        RE::NiPoint3 blastPos = a_target ? a_target->GetPosition() : a_caster->GetPosition();
        blastPos.z += 50.0f; // aim slightly above the target's feet

        // Fire a concentrated fireball toward the target position from just above the caster
        RE::NiPoint3 casterPos = a_caster->GetPosition();
        casterPos.z += 120.0f;

        LaunchFireball(a_caster, casterPos, blastPos);

        SKSE::log::info("ExplosionBlast: fired at ({:.1f},{:.1f},{:.1f})", blastPos.x, blastPos.y, blastPos.z);
        RE::DebugNotification("Blast fired!");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Player utility features
    // ──────────────────────────────────────────────────────────────────────────

    void RestorePlayer(RE::Actor* a_player)
    {
        SKSE::log::info("RestorePlayer: entry");
        if (!a_player) return;

        auto restore = [&](RE::ActorValue av) {
            float max = a_player->GetPermanentActorValue(av);
            a_player->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, av, max);
        };

        restore(RE::ActorValue::kHealth);
        restore(RE::ActorValue::kStamina);
        restore(RE::ActorValue::kMagicka);

        SKSE::log::info("RestorePlayer: health/stamina/magicka restored");
        RE::DebugNotification("Health, Stamina, and Magicka fully restored.");
    }

    void AddGold(RE::Actor* a_player)
    {
        SKSE::log::info("AddGold: entry");
        if (!a_player) return;

        auto* gold = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0000000F);
        if (!gold) {
            SKSE::log::error("AddGold: gold form not found");
            return;
        }

        a_player->AddObjectToContainer(gold, nullptr, 1000, nullptr);

        SKSE::log::info("AddGold: added 1000 gold");
        RE::DebugNotification("1000 Septims added to your inventory.");
    }

    void GiveWeapon(RE::Actor* a_player)
    {
        SKSE::log::info("GiveWeapon: entry");
        if (!a_player) return;

        // Ebony Sword (0x000139A4) — solid mid-tier weapon always in vanilla
        auto* weapon = RE::TESForm::LookupByID<RE::TESBoundObject>(0x000139A4);
        if (!weapon) {
            // Fallback: Iron Sword
            weapon = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00012EB7);
        }
        if (!weapon) {
            SKSE::log::error("GiveWeapon: weapon form not found");
            return;
        }

        a_player->AddObjectToContainer(weapon, nullptr, 1, nullptr);

        SKSE::log::info("GiveWeapon: gave '{}'", weapon->GetName());
        RE::DebugNotification(std::format("Received: {}", weapon->GetName()).c_str());
    }

    void InspectInventory(RE::Actor* a_player)
    {
        SKSE::log::info("InspectInventory: entry");
        if (!a_player) return;

        float health  = a_player->GetActorValue(RE::ActorValue::kHealth);
        float maxHp   = a_player->GetPermanentActorValue(RE::ActorValue::kHealth);
        float stamina = a_player->GetActorValue(RE::ActorValue::kStamina);
        float magicka = a_player->GetActorValue(RE::ActorValue::kMagicka);

        auto inv    = a_player->GetInventory();
        int  gold   = 0;
        int  items  = 0;
        for (auto& [form, data] : inv) {
            int cnt = data.first;
            if (form && form->formID == 0x0000000F) gold = cnt;
            items += cnt;
        }

        SKSE::log::info("InspectInventory: HP={:.0f}/{:.0f} Sta={:.0f} Mag={:.0f} Gold={} Items={}", health, maxHp, stamina, magicka, gold, items);

        RE::DebugNotification(std::format(
            "HP {:.0f}/{:.0f}  Sta {:.0f}  Mag {:.0f}  Gold {}  Items {}",
            health, maxHp, stamina, magicka, gold, items).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 2 features
    // ──────────────────────────────────────────────────────────────────────────

    void ToggleTimeFreeze()
    {
        SKSE::log::info("ToggleTimeFreeze: entry");

        auto* global = RE::TESForm::LookupByEditorID<RE::TESGlobal>("TimeScale");
        if (!global) {
            SKSE::log::error("ToggleTimeFreeze: TimeScale global not found");
            return;
        }

        // Normal Skyrim timescale is 20. We toggle between 20 and 0.2.
        static constexpr float kNormal  = 20.0f;
        static constexpr float kFrozen  = 0.2f;

        const bool isFrozen = global->value < 1.0f;
        global->value = isFrozen ? kNormal : kFrozen;

        SKSE::log::info("ToggleTimeFreeze: TimeScale -> {}", global->value);
        RE::DebugNotification(isFrozen ? "Time resumed." : "Time nearly frozen.");
    }

    void SpawnGuard(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnGuard: entry");
        if (!a_caster) return;

        // Try Whiterun Guard by EditorID, fallback to copying the player base
        auto* guardBase = RE::TESForm::LookupByEditorID<RE::TESNPC>("WhiterunGuard");
        if (!guardBase) guardBase = RE::TESForm::LookupByEditorID<RE::TESNPC>("WRGuardTemplateA");
        if (!guardBase) {
            // Absolute fallback: copy player NPC and name it Guardian
            auto* tmpl    = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);
            auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
            if (tmpl && factory) {
                guardBase = factory->Create()->As<RE::TESNPC>();
                if (guardBase) {
                    guardBase->Copy(tmpl);
                    guardBase->fullName = "Guardian";
                }
            }
        }

        if (!guardBase) {
            SKSE::log::error("SpawnGuard: no guard base found");
            return;
        }

        auto* ref = SpawnInFront(guardBase, a_caster, 180.0f);
        if (ref) {
            if (auto* actor = ref->As<RE::Actor>()) {
                actor->EvaluatePackage(false, true);
            }
            SKSE::log::info("SpawnGuard: '{}' spawned", guardBase->GetName());
            RE::DebugNotification(std::format("Guardian '{}' summoned.", guardBase->GetName()).c_str());
        }
    }

    void TeleportToCrosshair(RE::Actor* a_player)
    {
        SKSE::log::info("TeleportToCrosshair: entry");
        if (!a_player) return;

        auto* pick = RE::CrosshairPickData::GetSingleton();
        if (!pick) {
            SKSE::log::warn("TeleportToCrosshair: CrosshairPickData singleton null");
            return;
        }

        // collisionPoint is the exact surface the crosshair hit — nudge Z up a bit
        // so the player lands on top rather than inside the geometry.
        RE::NiPoint3 dest = pick->collisionPoint;
        dest.z += 60.0f;

        // Only teleport if the collision point is meaningfully far from the player
        // (prevents jitter when nothing is in crosshair).
        RE::NiPoint3 delta = dest - a_player->GetPosition();
        float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (distSq < 100.0f) {
            RE::DebugNotification("No valid destination in crosshair.");
            return;
        }

        a_player->SetPosition(dest);

        SKSE::log::info("TeleportToCrosshair: player moved to ({:.1f},{:.1f},{:.1f})", dest.x, dest.y, dest.z);
        RE::DebugNotification("Teleported!");
    }

    void HealTarget(RE::Actor* a_target)
    {
        SKSE::log::info("HealTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target) return;

        float maxHp = a_target->GetPermanentActorValue(RE::ActorValue::kHealth);
        a_target->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, maxHp);
        a_target->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kStamina, maxHp);

        SKSE::log::info("HealTarget: {} healed (max hp={:.1f})", a_target->GetName(), maxHp);
        RE::DebugNotification(std::format("{} healed.", a_target->GetName()).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 3 features
    // ──────────────────────────────────────────────────────────────────────────

    void ForcePush(RE::Actor* a_caster)
    {
        SKSE::log::info("ForcePush: entry");
        if (!a_caster) return;

        RE::NiPoint3 origin  = a_caster->GetPosition();
        constexpr float kRadius    = 600.0f;
        constexpr float kPushDist  = 400.0f;
        int count = 0;

        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;

        pl->ForAllActors([&](RE::Actor* actor) -> RE::BSContainer::ForEachResult {
            if (!actor || actor->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;

            RE::NiPoint3 delta = actor->GetPosition() - origin;
            float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            if (distSq > kRadius * kRadius || distSq < 0.001f) return RE::BSContainer::ForEachResult::kContinue;

            float dist = std::sqrt(distSq);
            RE::NiPoint3 dir = delta / dist;

            RE::NiPoint3 newPos = actor->GetPosition() + dir * kPushDist;
            newPos.z += 80.0f; // slight upward pop
            actor->SetPosition(newPos);
            actor->NotifyAnimationGraph("staggerStart");
            ++count;

            return RE::BSContainer::ForEachResult::kContinue;
        });

        SKSE::log::info("ForcePush: pushed {} actors", count);
        RE::DebugNotification(std::format("Force Push! {} actors blasted.", count).c_str());
    }

    void ToggleGodMode()
    {
        SKSE::log::info("ToggleGodMode: entry");

        // IsGodMode() reads from this global bool — we write to it directly.
        REL::Relocation<bool*> godModeFlag{ RELOCATION_ID(517711, 404238) };
        *godModeFlag = !(*godModeFlag);

        bool now = *godModeFlag;
        SKSE::log::info("ToggleGodMode: godMode={}", now);
        RE::DebugNotification(now ? "God Mode ON." : "God Mode OFF.");
    }

    void RandomTeleport(RE::Actor* a_player)
    {
        SKSE::log::info("RandomTeleport: entry");
        if (!a_player) return;

        static bool seeded = false;
        if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr))); seeded = true; }

        // Random direction (uniform angle), random distance 800–2500 units
        constexpr float kTwoPi = 6.2831853f;
        float angle = static_cast<float>(std::rand()) / RAND_MAX * kTwoPi;
        float dist  = 800.0f + static_cast<float>(std::rand() % 1700);

        RE::NiPoint3 pos = a_player->GetPosition();
        pos.x += std::sin(angle) * dist;
        pos.y += std::cos(angle) * dist;
        // keep Z — let the engine handle ground alignment on next tick

        a_player->SetPosition(pos);

        SKSE::log::info("RandomTeleport: moved {:.0f} units at angle {:.2f}rad", dist, angle);
        RE::DebugNotification(std::format("Random teleport! ({:.0f} units away)", dist).c_str());
    }

    void CloneObject(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("CloneObject: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target) return;

        auto* baseForm = a_target->GetBaseObject();
        if (!baseForm) {
            SKSE::log::warn("CloneObject: GetBaseObject() returned null");
            return;
        }

        // Place a clone adjacent to the original (offset by 80 units on X)
        auto* clone = a_target->PlaceObjectAtMe(baseForm, false);
        if (clone) {
            RE::NiPoint3 pos = a_target->GetPosition();
            pos.x += 80.0f;
            clone->SetPosition(pos);
            clone->data.angle = a_target->data.angle;

            SKSE::log::info("CloneObject: cloned '{}' at ({:.1f},{:.1f},{:.1f})", baseForm->GetName(), pos.x, pos.y, pos.z);
            RE::DebugNotification(std::format("Cloned: {}", baseForm->GetName()).c_str());
        } else {
            SKSE::log::warn("CloneObject: PlaceObjectAtMe returned null");
        }
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 4 features
    // ──────────────────────────────────────────────────────────────────────────

    void LevelUpPlayer(RE::PlayerCharacter* a_player)
    {
        SKSE::log::info("LevelUpPlayer: entry");
        if (!a_player) return;

        auto* skills = a_player->GetPlayerRuntimeData().skills;
        if (!skills) {
            SKSE::log::error("LevelUpPlayer: skills pointer is null");
            return;
        }

        skills->AdvanceLevel(true);

        SKSE::log::info("LevelUpPlayer: level advanced");
        RE::DebugNotification("Level Up!");
    }

    void DeleteTarget(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("DeleteTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target) return;

        // Never delete the player or a quest object
        if (a_target->IsPlayerRef()) {
            RE::DebugNotification("Cannot delete the player.");
            return;
        }
        if (a_target->HasQuestObject()) {
            RE::DebugNotification("Cannot delete a quest object.");
            return;
        }

        std::string name = a_target->GetName();
        a_target->Disable();
        a_target->SetDelete(true);

        SKSE::log::info("DeleteTarget: '{}' disabled and marked for deletion", name);
        RE::DebugNotification(std::format("Deleted: {}", name).c_str());
    }

    void ParalyzeTarget(RE::Actor* a_target)
    {
        SKSE::log::info("ParalyzeTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || a_target->IsPlayerRef()) return;

        // kParalysis (53): 1.0 = paralyzed, 0.0 = normal
        float current = a_target->GetActorValue(RE::ActorValue::kParalysis);
        float next    = (current > 0.5f) ? 0.0f : 1.0f;
        a_target->SetActorValue(RE::ActorValue::kParalysis, next);

        SKSE::log::info("ParalyzeTarget: {} paralysis {:.0f} -> {:.0f}", a_target->GetName(), current, next);
        RE::DebugNotification(
            next > 0.5f
                ? std::format("{} paralyzed.", a_target->GetName()).c_str()
                : std::format("{} unparalyzed.", a_target->GetName()).c_str());
    }

    void SpawnDragon(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnDragon: entry");
        if (!a_caster) return;

        // Try several EditorIDs; dragons vary by version/DLC
        RE::TESNPC* dragonBase = RE::TESForm::LookupByEditorID<RE::TESNPC>("MQ101Dragon");
        if (!dragonBase) dragonBase = RE::TESForm::LookupByEditorID<RE::TESNPC>("DragonAtronach01");
        if (!dragonBase) dragonBase = RE::TESForm::LookupByID<RE::TESNPC>(0x000D3A54); // Mirmulnir
        if (!dragonBase) dragonBase = RE::TESForm::LookupByID<RE::TESNPC>(0x0005A814); // Dragon01

        if (!dragonBase) {
            SKSE::log::error("SpawnDragon: no dragon form found");
            RE::DebugNotification("Dragon form not found in this load order.");
            return;
        }

        auto* ref = SpawnInFront(dragonBase, a_caster, 500.0f);
        if (ref) {
            if (auto* actor = ref->As<RE::Actor>()) {
                actor->EvaluatePackage(false, true);
            }
            SKSE::log::info("SpawnDragon: '{}' summoned", dragonBase->GetName());
            RE::DebugNotification(std::format("Dragon '{}' summoned!", dragonBase->GetName()).c_str());
        }
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 5 features
    // ──────────────────────────────────────────────────────────────────────────

    void ToggleWeather()
    {
        SKSE::log::info("ToggleWeather: entry");

        auto* sky = RE::Sky::GetSingleton();
        if (!sky) {
            SKSE::log::error("ToggleWeather: Sky singleton null");
            return;
        }

        // If we have an override weather active, clear it to restore default.
        if (sky->overrideWeather) {
            sky->ForceWeather(sky->defaultWeather ? sky->defaultWeather : sky->lastWeather, false);
            SKSE::log::info("ToggleWeather: restored default weather");
            RE::DebugNotification("Weather cleared.");
            return;
        }

        // Apply a storm — try several EditorIDs across vanilla + DLC
        auto* storm = RE::TESForm::LookupByEditorID<RE::TESWeather>("StormRain01");
        if (!storm) storm = RE::TESForm::LookupByEditorID<RE::TESWeather>("KynesgroveWeather01");
        if (!storm) storm = RE::TESForm::LookupByEditorID<RE::TESWeather>("SkyrimOvercastFog");

        if (!storm) {
            SKSE::log::error("ToggleWeather: no storm weather form found");
            RE::DebugNotification("Storm weather form not found.");
            return;
        }

        sky->ForceWeather(storm, true);
        SKSE::log::info("ToggleWeather: forced storm '{}'", storm->GetFormEditorID());
        RE::DebugNotification(std::format("Weather: {}", storm->GetFormEditorID()).c_str());
    }

    void FrenzyTarget(RE::Actor* a_target)
    {
        SKSE::log::info("FrenzyTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || a_target->IsPlayerRef()) return;

        a_target->SetActorValue(RE::ActorValue::kAggression, 3.0f);  // Frenzied
        a_target->SetActorValue(RE::ActorValue::kConfidence, 4.0f);  // Foolhardy
        a_target->SetActorValue(RE::ActorValue::kMorality, 0.0f);    // Any crime
        a_target->EvaluatePackage(true, true);

        SKSE::log::info("FrenzyTarget: {} frenzied", a_target->GetName());
        RE::DebugNotification(std::format("{} is now frenzied!", a_target->GetName()).c_str());
    }

    void LootTarget(RE::TESObjectREFR* a_target, RE::Actor* a_player)
    {
        SKSE::log::info("LootTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || !a_player || a_target->IsPlayerRef()) return;

        auto inv = a_target->GetInventory();
        if (inv.empty()) {
            RE::DebugNotification("Nothing to loot.");
            return;
        }

        int transferred = 0;
        for (auto& [form, data] : inv) {
            int count = data.first;
            if (form && count > 0) {
                a_target->RemoveItem(form, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, a_player);
                ++transferred;
                SKSE::log::info("  Looted: {} x{}", form->GetName(), count);
            }
        }

        SKSE::log::info("LootTarget: transferred {} item types from '{}'", transferred, a_target->GetName());
        RE::DebugNotification(std::format("Looted {} item type(s) from {}.", transferred, a_target->GetName()).c_str());
    }

    void ToggleSpeedBoost(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleSpeedBoost: entry");
        if (!a_player) return;

        constexpr float kNormal = 100.0f;
        constexpr float kFast   = 300.0f;

        float cur  = a_player->GetActorValue(RE::ActorValue::kSpeedMult);
        float next = (cur >= kFast - 1.0f) ? kNormal : kFast;

        // Use ForceActorValue so it bypasses clamp logic for the base value.
        a_player->SetActorValue(RE::ActorValue::kSpeedMult, next);

        SKSE::log::info("ToggleSpeedBoost: SpeedMult {:.0f} -> {:.0f}", cur, next);
        RE::DebugNotification(next > kNormal ? "Speed Boost ON (3x)!" : "Speed restored.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Spell-cast event dispatcher
    // ──────────────────────────────────────────────────────────────────────────

    class SpellCastHandler : public RE::BSTEventSink<RE::TESSpellCastEvent>
    {
    public:
        static SpellCastHandler* GetSingleton()
        {
            static SpellCastHandler singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::TESSpellCastEvent*                 a_event,
            RE::BSTEventSource<RE::TESSpellCastEvent>*)  override
        {
            if (!a_event) return RE::BSEventNotifyControl::kContinue;

            auto* casterRef = a_event->object.get();
            if (!casterRef) return RE::BSEventNotifyControl::kContinue;

            auto* caster = casterRef->As<RE::Actor>();
            if (!caster || !caster->IsPlayerRef()) return RE::BSEventNotifyControl::kContinue;

            RE::FormID spellID = a_event->spell;

            auto* crossRef = GetCrosshairRef();

            auto Match = [&](RE::SpellItem* spell) -> bool {
                return spell && spell->GetFormID() == spellID;
            };

            SKSE::log::debug("SpellCast: FormID={:#010x}", spellID);

            if (Match(g_spawnCitizenSpell)) {
                SpawnCitizen(caster);
            } else if (Match(g_tameNpcSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) TameNPC(target);
                else RE::DebugNotification("No NPC in crosshair to tame.");
            } else if (Match(g_cycleScaleSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) CycleNPCScale(target);
                else RE::DebugNotification("No NPC in crosshair to resize.");
            } else if (Match(g_spawnTreeSpell)) {
                SpawnTree(caster);
            } else if (Match(g_spawnRockSpell)) {
                SpawnRock(caster);
            } else if (Match(g_raiseObjectSpell)) {
                if (crossRef) RaiseObject(crossRef);
                else RE::DebugNotification("No object in crosshair to raise.");
            } else if (Match(g_lowerObjectSpell)) {
                if (crossRef) LowerObject(crossRef);
                else RE::DebugNotification("No object in crosshair to lower.");
            } else if (Match(g_meteorRainSpell)) {
                MeteorRain(caster);
            } else if (Match(g_explosionSpell)) {
                ExplosionBlast(crossRef, caster);
            } else if (Match(g_restoreSpell)) {
                RestorePlayer(caster);
            } else if (Match(g_addGoldSpell)) {
                AddGold(caster);
            } else if (Match(g_giveWeaponSpell)) {
                GiveWeapon(caster);
            } else if (Match(g_inspectSpell)) {
                InspectInventory(caster);
            } else if (Match(g_timeFreezeSpell)) {
                ToggleTimeFreeze();
            } else if (Match(g_spawnGuardSpell)) {
                SpawnGuard(caster);
            } else if (Match(g_teleportSpell)) {
                TeleportToCrosshair(caster);
            } else if (Match(g_healTargetSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) HealTarget(target);
                else RE::DebugNotification("No NPC in crosshair to heal.");
            } else if (Match(g_forcePushSpell)) {
                ForcePush(caster);
            } else if (Match(g_godModeSpell)) {
                ToggleGodMode();
            } else if (Match(g_randomTeleSpell)) {
                RandomTeleport(caster);
            } else if (Match(g_cloneObjectSpell)) {
                if (crossRef) CloneObject(crossRef);
                else RE::DebugNotification("No object in crosshair to clone.");
            } else if (Match(g_levelUpSpell)) {
                LevelUpPlayer(RE::PlayerCharacter::GetSingleton());
            } else if (Match(g_deleteTargetSpell)) {
                if (crossRef) DeleteTarget(crossRef);
                else RE::DebugNotification("No object in crosshair to delete.");
            } else if (Match(g_paralyzeSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) ParalyzeTarget(target);
                else RE::DebugNotification("No NPC in crosshair to paralyze.");
            } else if (Match(g_spawnDragonSpell)) {
                SpawnDragon(caster);
            } else if (Match(g_weatherSpell)) {
                ToggleWeather();
            } else if (Match(g_frenzySpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) FrenzyTarget(target);
                else RE::DebugNotification("No NPC in crosshair to frenzy.");
            } else if (Match(g_lootSpell)) {
                if (crossRef) LootTarget(crossRef, caster);
                else RE::DebugNotification("No target in crosshair to loot.");
            } else if (Match(g_speedSpell)) {
                ToggleSpeedBoost(caster);
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    // ──────────────────────────────────────────────────────────────────────────
    // Lifecycle
    // ──────────────────────────────────────────────────────────────────────────

    void InitializeMagic()
    {
        SKSE::log::info("MagicToolkit: InitializeMagic()");

        auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();
        if (!factory) {
            SKSE::log::error("MagicToolkit: SpellItem factory not found");
            return;
        }

        auto MakeSpell = [&](RE::SpellItem*& out, const char* name) {
            out = factory->Create()->As<RE::SpellItem>();
            if (out) {
                out->fullName               = name;
                out->data.spellType         = RE::MagicSystem::SpellType::kLesserPower;
                out->data.castingType       = RE::MagicSystem::CastingType::kFireAndForget;
                out->data.delivery          = RE::MagicSystem::Delivery::kSelf;
                SKSE::log::info("  Created spell: '{}' id={:#010x}", name, out->GetFormID());
            } else {
                SKSE::log::error("  Failed to create spell: '{}'", name);
            }
        };

        MakeSpell(g_spawnCitizenSpell,  "[C++] Spawn Citizen");
        MakeSpell(g_tameNpcSpell,       "[C++] Tame NPC");
        MakeSpell(g_cycleScaleSpell,    "[C++] Cycle NPC Scale");
        MakeSpell(g_spawnTreeSpell,     "[C++] Spawn Tree");
        MakeSpell(g_spawnRockSpell,     "[C++] Spawn Rock");
        MakeSpell(g_raiseObjectSpell,   "[C++] Raise Object");
        MakeSpell(g_lowerObjectSpell,   "[C++] Lower Object");
        MakeSpell(g_meteorRainSpell,    "[C++] Meteor Rain");
        MakeSpell(g_explosionSpell,     "[C++] Explosion Blast");
        MakeSpell(g_restoreSpell,       "[C++] Restore Vitals");
        MakeSpell(g_addGoldSpell,       "[C++] Add Gold");
        MakeSpell(g_giveWeaponSpell,    "[C++] Give Weapon");
        MakeSpell(g_inspectSpell,       "[C++] Inspect Stats");
        // Wave 2
        MakeSpell(g_timeFreezeSpell,    "[C++] Toggle Time Freeze");
        MakeSpell(g_spawnGuardSpell,    "[C++] Spawn Guardian");
        MakeSpell(g_teleportSpell,      "[C++] Teleport to Crosshair");
        MakeSpell(g_healTargetSpell,    "[C++] Heal Target");
        // Wave 3
        MakeSpell(g_forcePushSpell,     "[C++] Force Push");
        MakeSpell(g_godModeSpell,       "[C++] Toggle God Mode");
        MakeSpell(g_randomTeleSpell,    "[C++] Random Teleport");
        MakeSpell(g_cloneObjectSpell,   "[C++] Clone Object");
        // Wave 4
        MakeSpell(g_levelUpSpell,       "[C++] Level Up");
        MakeSpell(g_deleteTargetSpell,  "[C++] Delete Target");
        MakeSpell(g_paralyzeSpell,      "[C++] Toggle Paralyze");
        MakeSpell(g_spawnDragonSpell,   "[C++] Summon Dragon");
        // Wave 5
        MakeSpell(g_weatherSpell,       "[C++] Toggle Storm");
        MakeSpell(g_frenzySpell,        "[C++] Frenzy Target");
        MakeSpell(g_lootSpell,          "[C++] Loot Target");
        MakeSpell(g_speedSpell,         "[C++] Speed Boost");

        auto* source = RE::ScriptEventSourceHolder::GetSingleton();
        if (source) {
            source->AddEventSink<RE::TESSpellCastEvent>(SpellCastHandler::GetSingleton());
            SKSE::log::info("MagicToolkit: event sink registered");
        } else {
            SKSE::log::error("MagicToolkit: ScriptEventSourceHolder not found");
        }
    }

    void GiveSpellsToPlayer()
    {
        SKSE::log::info("MagicToolkit: GiveSpellsToPlayer()");

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            SKSE::log::warn("MagicToolkit: player singleton is null");
            return;
        }

        RE::SpellItem* spells[] = {
            g_spawnCitizenSpell,
            g_tameNpcSpell,
            g_cycleScaleSpell,
            g_spawnTreeSpell,
            g_spawnRockSpell,
            g_raiseObjectSpell,
            g_lowerObjectSpell,
            g_meteorRainSpell,
            g_explosionSpell,
            g_restoreSpell,
            g_addGoldSpell,
            g_giveWeaponSpell,
            g_inspectSpell,
            // Wave 2
            g_timeFreezeSpell,
            g_spawnGuardSpell,
            g_teleportSpell,
            g_healTargetSpell,
            // Wave 3
            g_forcePushSpell,
            g_godModeSpell,
            g_randomTeleSpell,
            g_cloneObjectSpell,
            // Wave 4
            g_levelUpSpell,
            g_deleteTargetSpell,
            g_paralyzeSpell,
            g_spawnDragonSpell,
            // Wave 5
            g_weatherSpell,
            g_frenzySpell,
            g_lootSpell,
            g_speedSpell,
        };

        for (auto* spell : spells) {
            if (!spell) continue;
            if (!player->HasSpell(spell)) {
                player->AddSpell(spell);
                SKSE::log::info("  Gave spell '{}' to player", spell->fullName.c_str());
            }
        }
    }
}
