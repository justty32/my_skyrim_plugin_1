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
    // Wave 6
    RE::SpellItem* g_gravityPullSpell   = nullptr;
    RE::SpellItem* g_skeletonArmySpell  = nullptr;
    RE::SpellItem* g_launchSpell        = nullptr;
    RE::SpellItem* g_skillBoostSpell    = nullptr;
    // Wave 7
    RE::SpellItem* g_pacifyAllSpell     = nullptr;
    RE::SpellItem* g_identifySpell      = nullptr;
    RE::SpellItem* g_resurrectSpell     = nullptr;
    RE::SpellItem* g_armorBoostSpell    = nullptr;
    // Wave 8
    RE::SpellItem* g_markRecallSpell    = nullptr;
    RE::SpellItem* g_playerGiantSpell   = nullptr;
    RE::SpellItem* g_massHealSpell      = nullptr;
    RE::SpellItem* g_drainTargetSpell   = nullptr;
    // Wave 9
    RE::SpellItem* g_gatherNpcsSpell    = nullptr;
    RE::SpellItem* g_carrySpell         = nullptr;
    RE::SpellItem* g_frostBarrageSpell  = nullptr;
    RE::SpellItem* g_snapGroundSpell    = nullptr;
    // Wave 10
    RE::SpellItem* g_miniaturizeSpell   = nullptr;
    RE::SpellItem* g_executeSpell       = nullptr;
    RE::SpellItem* g_bounceHouseSpell   = nullptr;
    RE::SpellItem* g_detectionSpell     = nullptr;
    // Wave 11
    RE::SpellItem* g_massFreezeSpell      = nullptr;
    RE::SpellItem* g_infiniteMagickaSpell = nullptr;
    RE::SpellItem* g_deathZoneSpell       = nullptr;
    RE::SpellItem* g_superJumpSpell       = nullptr;
    // Wave 12
    RE::SpellItem* g_cloneNpcSpell        = nullptr;
    RE::SpellItem* g_swapPosSpell         = nullptr;
    RE::SpellItem* g_massFrenzySpell      = nullptr;
    RE::SpellItem* g_scatterSpell         = nullptr;
    // Wave 13
    RE::SpellItem* g_waterBreathSpell     = nullptr;
    RE::SpellItem* g_unlockSpell          = nullptr;
    RE::SpellItem* g_countEnemiesSpell    = nullptr;
    RE::SpellItem* g_timeStopSpell        = nullptr;
    // Wave 14
    RE::SpellItem* g_infernoSpell         = nullptr;
    RE::SpellItem* g_atronachSpell        = nullptr;
    RE::SpellItem* g_cloneArmySpell       = nullptr;
    RE::SpellItem* g_giveArmorSpell       = nullptr;
    // Wave 15
    RE::SpellItem* g_magickaStealSpell    = nullptr;
    RE::SpellItem* g_massExecuteSpell     = nullptr;
    RE::SpellItem* g_merchantSpell        = nullptr;
    RE::SpellItem* g_magickaRegenSpell    = nullptr;
    // Wave 16
    RE::SpellItem* g_staminaSurgeSpell    = nullptr;
    RE::SpellItem* g_areaStaggerSpell     = nullptr;
    RE::SpellItem* g_goldAuraSpell        = nullptr;
    RE::SpellItem* g_detectLifeSpell      = nullptr;
    // Wave 17
    RE::SpellItem* g_disarmSpell          = nullptr;
    RE::SpellItem* g_refillArrowsSpell    = nullptr;
    RE::SpellItem* g_massLevitateSpell    = nullptr;
    RE::SpellItem* g_soulTrapAuraSpell    = nullptr;
    // Wave 18
    RE::SpellItem* g_stealHealthSpell     = nullptr;
    RE::SpellItem* g_massUnlockSpell      = nullptr;
    RE::SpellItem* g_shieldBashSpell      = nullptr;
    RE::SpellItem* g_blizzardSpell        = nullptr;
    // Wave 19
    RE::SpellItem* g_ragdollAllSpell      = nullptr;
    RE::SpellItem* g_equipBestSpell       = nullptr;
    RE::SpellItem* g_massResurrectSpell   = nullptr;
    RE::SpellItem* g_oblivionGateSpell    = nullptr;
    // Wave 20
    RE::SpellItem* g_ghostFormSpell       = nullptr;
    RE::SpellItem* g_worldquakeSpell      = nullptr;
    RE::SpellItem* g_massParalyzeSpell    = nullptr;
    RE::SpellItem* g_spawnWolfPackSpell   = nullptr;
    // Wave 21
    RE::SpellItem* g_anchorTargetSpell    = nullptr;
    RE::SpellItem* g_shrinkTargetSpell    = nullptr;
    RE::SpellItem* g_nightEyeSpell        = nullptr;
    RE::SpellItem* g_massSilenceSpell     = nullptr;
    // Wave 22
    RE::SpellItem* g_spawnHorseSpell      = nullptr;
    RE::SpellItem* g_pickpocketAllSpell   = nullptr;
    RE::SpellItem* g_massSlowSpell        = nullptr;
    RE::SpellItem* g_spawnChestSpell      = nullptr;
    // Wave 23
    RE::SpellItem* g_berserkSpell         = nullptr;
    RE::SpellItem* g_spawnDwarvenSpell    = nullptr;
    RE::SpellItem* g_tornadoSpell         = nullptr;
    RE::SpellItem* g_petrifySpell         = nullptr;
    // Wave 24
    RE::SpellItem* g_apocalypseSpell      = nullptr;
    RE::SpellItem* g_massDisarmSpell      = nullptr;
    RE::SpellItem* g_dragonShoutSpell     = nullptr;
    RE::SpellItem* g_essenceAbsorbSpell   = nullptr;

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
    // Wave 6 features
    // ──────────────────────────────────────────────────────────────────────────

    void GravityPull(RE::Actor* a_caster)
    {
        SKSE::log::info("GravityPull: entry");
        if (!a_caster) return;

        RE::NiPoint3 origin = a_caster->GetPosition();
        constexpr float kRadius   = 600.0f;
        constexpr float kPullDist = 350.0f;
        int count = 0;

        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;

        pl->ForAllActors([&](RE::Actor* actor) -> RE::BSContainer::ForEachResult {
            if (!actor || actor->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;

            RE::NiPoint3 delta = actor->GetPosition() - origin;
            float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            if (distSq > kRadius * kRadius || distSq < 1.0f) return RE::BSContainer::ForEachResult::kContinue;

            float dist = std::sqrt(distSq);
            RE::NiPoint3 toPlayer = (origin - actor->GetPosition()) / dist;

            // Pull toward player but stop 80 units away to avoid overlap
            float pull = std::min(kPullDist, dist - 80.0f);
            if (pull <= 0.0f) return RE::BSContainer::ForEachResult::kContinue;

            RE::NiPoint3 newPos = actor->GetPosition() + toPlayer * pull;
            actor->SetPosition(newPos);
            ++count;

            return RE::BSContainer::ForEachResult::kContinue;
        });

        SKSE::log::info("GravityPull: pulled {} actors toward player", count);
        RE::DebugNotification(std::format("Gravity Pull! {} actors dragged in.", count).c_str());
    }

    void SpawnSkeletonArmy(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnSkeletonArmy: entry");
        if (!a_caster) return;

        // Try vanilla skeleton, then draugr, then player-copy fallback
        auto* skelBase = RE::TESForm::LookupByEditorID<RE::TESNPC>("EncSkeletonWarrior01");
        if (!skelBase) skelBase = RE::TESForm::LookupByEditorID<RE::TESNPC>("EncDraugr01");
        if (!skelBase) skelBase = RE::TESForm::LookupByEditorID<RE::TESNPC>("DraugrMale01");

        bool isFallback = false;
        RE::TESNPC* fallbackBase = nullptr;
        if (!skelBase) {
            auto* tmpl    = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);
            auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
            if (tmpl && factory) {
                fallbackBase = factory->Create()->As<RE::TESNPC>();
                if (fallbackBase) { fallbackBase->Copy(tmpl); fallbackBase->fullName = "Skeleton"; }
                skelBase   = fallbackBase;
                isFallback = true;
            }
        }

        if (!skelBase) {
            SKSE::log::error("SpawnSkeletonArmy: no skeleton form found");
            return;
        }

        constexpr int   kCount  = 3;
        constexpr float kRadius = 220.0f;
        constexpr float kTwoPi  = 6.2831853f;
        float baseAngle = a_caster->data.angle.z;

        int spawned = 0;
        for (int i = 0; i < kCount; ++i) {
            float angle = baseAngle + (kTwoPi / kCount) * i;
            RE::NiPoint3 pos = a_caster->GetPosition();
            pos.x += std::sin(angle) * kRadius;
            pos.y += std::cos(angle) * kRadius;

            auto* ref = a_caster->PlaceObjectAtMe(skelBase, false);
            if (ref) {
                ref->SetPosition(pos);
                if (auto* actor = ref->As<RE::Actor>()) {
                    actor->EvaluatePackage(false, true);
                }
                ++spawned;
            }
        }

        SKSE::log::info("SpawnSkeletonArmy: spawned {} '{}' units", spawned, skelBase->GetName());
        RE::DebugNotification(std::format("Skeleton army ({})!", spawned).c_str());
    }

    void LaunchTarget(RE::Actor* a_target)
    {
        SKSE::log::info("LaunchTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || a_target->IsPlayerRef()) return;

        RE::NiPoint3 pos = a_target->GetPosition();
        pos.z += 1800.0f;
        a_target->SetPosition(pos);

        SKSE::log::info("LaunchTarget: '{}' launched to z={:.0f}", a_target->GetName(), pos.z);
        RE::DebugNotification(std::format("{} launched!", a_target->GetName()).c_str());
    }

    void ToggleSkillBoost(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleSkillBoost: entry");
        if (!a_player) return;

        // Check current One-Handed to detect boosted state (base skill capped at 100 normally)
        constexpr float kMax   = 100.0f;
        constexpr float kReset = 15.0f;

        float cur = a_player->GetActorValue(RE::ActorValue::kOneHanded);
        bool  isBoosted = cur >= kMax - 0.5f;

        float target = isBoosted ? kReset : kMax;

        static const RE::ActorValue kSkills[] = {
            RE::ActorValue::kOneHanded,
            RE::ActorValue::kTwoHanded,
            RE::ActorValue::kArchery,
            RE::ActorValue::kBlock,
            RE::ActorValue::kHeavyArmor,
            RE::ActorValue::kLightArmor,
            RE::ActorValue::kSneak,
            RE::ActorValue::kLockpicking,
            RE::ActorValue::kPickpocket,
            RE::ActorValue::kSpeech,
            RE::ActorValue::kAlteration,
            RE::ActorValue::kConjuration,
            RE::ActorValue::kDestruction,
            RE::ActorValue::kIllusion,
            RE::ActorValue::kRestoration,
            RE::ActorValue::kEnchanting,
            RE::ActorValue::kSmithing,
            RE::ActorValue::kAlchemy,
        };

        for (auto av : kSkills) {
            a_player->SetActorValue(av, target);
        }

        SKSE::log::info("ToggleSkillBoost: all skills -> {:.0f}", target);
        RE::DebugNotification(isBoosted ? "Skills reset to 15." : "All skills maxed to 100!");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 7 features
    // ──────────────────────────────────────────────────────────────────────────

    void PacifyAll()
    {
        SKSE::log::info("PacifyAll: entry");
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;

        int count = 0;
        pl->ForAllActors([&](RE::Actor* actor) -> RE::BSContainer::ForEachResult {
            if (!actor || actor->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            pl->StopCombatAndAlarmOnActor(actor, true);
            actor->SetActorValue(RE::ActorValue::kAggression, 0.0f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });

        SKSE::log::info("PacifyAll: pacified {} actors", count);
        RE::DebugNotification(std::format("Peace! {} actors calmed.", count).c_str());
    }

    void IdentifyTarget(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("IdentifyTarget: entry");
        if (!a_target) return;

        auto* base = a_target->GetBaseObject();
        auto* cell = a_target->GetParentCell();

        std::string info = std::format(
            "{} | FormID:{:#010x} | Base:{} ({:#010x}) | Cell:{}",
            a_target->GetName(),
            a_target->GetFormID(),
            base ? base->GetName() : "?",
            base ? base->GetFormID() : 0u,
            cell ? cell->GetName() : "?");

        SKSE::log::info("IdentifyTarget: {}", info);
        RE::DebugNotification(info.c_str());
    }

    void ResurrectTarget(RE::Actor* a_target)
    {
        SKSE::log::info("ResurrectTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || a_target->IsPlayerRef()) return;

        if (!a_target->IsDead()) {
            RE::DebugNotification(std::format("{} is not dead.", a_target->GetName()).c_str());
            return;
        }

        a_target->Resurrect(false, true);
        a_target->EvaluatePackage(false, true);

        SKSE::log::info("ResurrectTarget: {} resurrected", a_target->GetName());
        RE::DebugNotification(std::format("{} lives again!", a_target->GetName()).c_str());
    }

    void ToggleArmorBoost(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleArmorBoost: entry");
        if (!a_player) return;

        constexpr float kBoosted = 10000.0f; // effectively immune
        constexpr float kNormal  = 0.0f;

        float cur  = a_player->GetActorValue(RE::ActorValue::kDamageResist);
        float next = (cur >= kBoosted - 1.0f) ? kNormal : kBoosted;
        a_player->SetActorValue(RE::ActorValue::kDamageResist, next);

        SKSE::log::info("ToggleArmorBoost: DamageResist {} -> {}", cur, next);
        RE::DebugNotification(next > 0.0f ? "Iron Skin: ON (immune to damage)." : "Iron Skin: OFF.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 8 features
    // ──────────────────────────────────────────────────────────────────────────

    // Mark/Recall: static storage for marked position (valid after first cast)
    static bool           s_hasMark   = false;
    static RE::NiPoint3   s_markPos   = {};
    static RE::TESObjectCELL* s_markCell = nullptr;

    void MarkOrRecall(RE::Actor* a_player)
    {
        SKSE::log::info("MarkOrRecall: entry, hasMark={}", s_hasMark);
        if (!a_player) return;

        if (!s_hasMark) {
            // First cast: save position
            s_markPos  = a_player->GetPosition();
            s_markCell = a_player->GetParentCell();
            s_hasMark  = true;
            SKSE::log::info("MarkOrRecall: marked at ({:.1f},{:.1f},{:.1f})", s_markPos.x, s_markPos.y, s_markPos.z);
            RE::DebugNotification("Position marked. Cast again to recall.");
        } else {
            // Second cast: teleport back, then clear mark
            a_player->SetPosition(s_markPos);
            s_hasMark = false;
            SKSE::log::info("MarkOrRecall: recalled to ({:.1f},{:.1f},{:.1f})", s_markPos.x, s_markPos.y, s_markPos.z);
            RE::DebugNotification("Recalled to marked position!");
        }
    }

    void CyclePlayerScale(RE::Actor* a_player)
    {
        SKSE::log::info("CyclePlayerScale: entry");
        if (!a_player) return;

        static const float kScales[] = { 1.0f, 2.0f, 4.0f, 0.5f };
        static const int   kCount    = static_cast<int>(std::size(kScales));

        float cur     = a_player->GetScale();
        int   nextIdx = 0;
        for (int i = 0; i < kCount; ++i) {
            if (std::abs(cur - kScales[i]) < 0.1f) { nextIdx = (i + 1) % kCount; break; }
        }

        float next = kScales[nextIdx];
        a_player->GetReferenceRuntimeData().refScale = static_cast<std::uint16_t>(next * 100.0f);
        a_player->DoReset3D(true);

        SKSE::log::info("CyclePlayerScale: {:.1f} -> {:.1f}", cur, next);
        RE::DebugNotification(std::format("Player scale: {:.1f}x", next).c_str());
    }

    void MassHealNearby()
    {
        SKSE::log::info("MassHealNearby: entry");
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        RE::NiPoint3 origin = player->GetPosition();
        constexpr float kRadius = 800.0f;
        int count = 0;

        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;

        pl->ForAllActors([&](RE::Actor* actor) -> RE::BSContainer::ForEachResult {
            RE::NiPoint3 delta = actor->GetPosition() - origin;
            float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            if (distSq > kRadius * kRadius) return RE::BSContainer::ForEachResult::kContinue;

            float maxHp = actor->GetPermanentActorValue(RE::ActorValue::kHealth);
            actor->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, maxHp);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });

        SKSE::log::info("MassHealNearby: healed {} actors", count);
        RE::DebugNotification(std::format("Mass Heal! {} actors restored.", count).c_str());
    }

    void DrainTarget(RE::Actor* a_target)
    {
        SKSE::log::info("DrainTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || a_target->IsPlayerRef()) return;

        float cur  = a_target->GetActorValue(RE::ActorValue::kHealth);
        float drain = cur * 0.5f; // drain 50 % of current HP

        if (drain < 1.0f) {
            RE::DebugNotification(std::format("{} is nearly dead already.", a_target->GetName()).c_str());
            return;
        }

        // Negative restore = damage
        a_target->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -drain);

        // Give the drained HP to the player
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            player->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, drain);
        }

        SKSE::log::info("DrainTarget: drained {:.0f} HP from {}", drain, a_target->GetName());
        RE::DebugNotification(std::format("Drained {:.0f} HP from {}!", drain, a_target->GetName()).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 9 features
    // ──────────────────────────────────────────────────────────────────────────

    void GatherNPCs(RE::Actor* a_caster)
    {
        SKSE::log::info("GatherNPCs: entry");
        if (!a_caster) return;

        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;

        // Collect nearby actors first so we can assign ring positions
        std::vector<RE::Actor*> nearby;
        RE::NiPoint3 origin = a_caster->GetPosition();

        pl->ForAllActors([&](RE::Actor* actor) -> RE::BSContainer::ForEachResult {
            if (!actor || actor->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = actor->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z < 1000.0f * 1000.0f) nearby.push_back(actor);
            return RE::BSContainer::ForEachResult::kContinue;
        });

        constexpr float kTwoPi = 6.2831853f;
        float ringRadius = 160.0f + nearby.size() * 15.0f; // expand ring with crowd size
        for (int i = 0; i < static_cast<int>(nearby.size()); ++i) {
            float angle = kTwoPi * i / static_cast<float>(nearby.size());
            RE::NiPoint3 pos = origin;
            pos.x += std::sin(angle) * ringRadius;
            pos.y += std::cos(angle) * ringRadius;
            nearby[i]->SetPosition(pos);
        }

        SKSE::log::info("GatherNPCs: gathered {} actors into ring r={:.0f}", nearby.size(), ringRadius);
        RE::DebugNotification(std::format("Gathered {} NPCs around you!", nearby.size()).c_str());
    }

    void ToggleUnlimitedCarry(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleUnlimitedCarry: entry");
        if (!a_player) return;

        constexpr float kUnlimited = 999999.0f;
        float cur = a_player->GetActorValue(RE::ActorValue::kCarryWeight);

        if (cur >= kUnlimited - 1.0f) {
            // Restore base carry weight
            float base = a_player->GetPermanentActorValue(RE::ActorValue::kCarryWeight);
            a_player->SetActorValue(RE::ActorValue::kCarryWeight, base);
            SKSE::log::info("ToggleUnlimitedCarry: restored base carry {:.0f}", base);
            RE::DebugNotification(std::format("Carry weight restored ({:.0f}).", base).c_str());
        } else {
            a_player->SetActorValue(RE::ActorValue::kCarryWeight, kUnlimited);
            SKSE::log::info("ToggleUnlimitedCarry: set to unlimited");
            RE::DebugNotification("Unlimited carry weight ON!");
        }
    }

    void FrostBarrage(RE::Actor* a_caster)
    {
        SKSE::log::info("FrostBarrage: entry");
        if (!a_caster) return;

        // Try ice/frost projectile, fall back to fireball
        auto* projBase = RE::TESForm::LookupByEditorID<RE::BGSProjectile>("MagicIceSpikeProjectile01");
        if (!projBase) projBase = RE::TESForm::LookupByEditorID<RE::BGSProjectile>("MagicIceStormProjectile01");
        if (!projBase) projBase = RE::TESForm::LookupByID<RE::BGSProjectile>(0x00015CFD); // fireball fallback

        if (!projBase) {
            SKSE::log::error("FrostBarrage: no projectile form found");
            return;
        }

        // 8 projectiles fanned across ±45° in the player's facing direction
        constexpr int   kCount    = 8;
        constexpr float kSpread   = 1.5708f / 2.0f; // 45° total half-spread
        float baseYaw = a_caster->data.angle.z;

        RE::NiPoint3 origin = a_caster->GetPosition();
        origin.z += 120.0f; // fire from chest height

        for (int i = 0; i < kCount; ++i) {
            float t   = (kCount > 1) ? static_cast<float>(i) / (kCount - 1) : 0.5f;
            float yaw = baseYaw + kSpread * (t * 2.0f - 1.0f);

            RE::Projectile::LaunchData ld;
            ld.origin         = origin;
            ld.projectileBase = projBase;
            ld.shooter        = a_caster;
            ld.angleZ         = yaw;
            ld.angleX         = 0.0f;
            ld.parentCell     = a_caster->GetParentCell();
            ld.castingSource  = RE::MagicSystem::CastingSource::kRightHand;
            ld.power          = 1.0f;
            ld.scale          = 1.0f;
            ld.useOrigin      = true;
            ld.autoAim        = false;

            RE::ProjectileHandle result;
            RE::Projectile::Launch(&result, ld);
        }

        SKSE::log::info("FrostBarrage: fired {} projectiles", kCount);
        RE::DebugNotification("Frost Barrage!");
    }

    void SnapToGround(RE::TESObjectREFR* a_target, RE::Actor* a_caster)
    {
        SKSE::log::info("SnapToGround: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || !a_caster) return;

        RE::NiPoint3 pos = a_target->GetPosition();
        pos.z = a_caster->GetPosition().z;
        a_target->SetPosition(pos);

        SKSE::log::info("SnapToGround: '{}' snapped to z={:.1f}", a_target->GetName(), pos.z);
        RE::DebugNotification(std::format("{} snapped to ground.", a_target->GetName()).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 10 features
    // ──────────────────────────────────────────────────────────────────────────

    void ToggleMiniaturizeAll(RE::Actor* a_caster)
    {
        SKSE::log::info("ToggleMiniaturizeAll: entry");

        static bool s_miniaturized = false;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;

        RE::NiPoint3 origin = a_caster->GetPosition();
        std::uint16_t targetScale = s_miniaturized ? 100 : 20; // 1.0x or 0.2x
        int count = 0;

        pl->ForAllActors([&](RE::Actor* actor) -> RE::BSContainer::ForEachResult {
            if (!actor || actor->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = actor->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 800.0f * 800.0f) return RE::BSContainer::ForEachResult::kContinue;
            actor->GetReferenceRuntimeData().refScale = targetScale;
            actor->DoReset3D(true);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });

        s_miniaturized = !s_miniaturized;
        SKSE::log::info("ToggleMiniaturizeAll: {} actors -> {:.1f}x", count, targetScale / 100.0f);
        RE::DebugNotification(s_miniaturized
            ? std::format("{} NPCs miniaturized (0.2x)!", count).c_str()
            : std::format("{} NPCs restored to normal size.", count).c_str());
    }

    void ExecuteTarget(RE::Actor* a_target)
    {
        SKSE::log::info("ExecuteTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target || a_target->IsPlayerRef()) return;

        std::string name = a_target->GetName();
        // Deal enough negative health to kill regardless of max HP
        a_target->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -99999.0f);

        SKSE::log::info("ExecuteTarget: '{}' executed", name);
        RE::DebugNotification(std::format("{} executed!", name).c_str());
    }

    void BounceHouse(RE::Actor* a_caster)
    {
        SKSE::log::info("BounceHouse: entry");
        if (!a_caster) return;

        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;

        RE::NiPoint3 origin = a_caster->GetPosition();
        constexpr float kRadius = 500.0f;
        int count = 0;

        pl->ForAllActors([&](RE::Actor* actor) -> RE::BSContainer::ForEachResult {
            if (!actor || actor->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = actor->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > kRadius * kRadius) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 pos = actor->GetPosition();
            pos.z += 1000.0f;
            actor->SetPosition(pos);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });

        SKSE::log::info("BounceHouse: launched {} actors", count);
        RE::DebugNotification(std::format("Bounce House! {} launched!", count).c_str());
    }

    void ToggleDetection()
    {
        SKSE::log::info("ToggleDetection: entry");

        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) {
            SKSE::log::error("ToggleDetection: ProcessLists singleton null");
            return;
        }

        pl->runDetection = !pl->runDetection;

        SKSE::log::info("ToggleDetection: runDetection={}", pl->runDetection);
        RE::DebugNotification(pl->runDetection
            ? "Detection ON — NPCs can see you."
            : "Detection OFF — you are undetectable.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 11
    // ──────────────────────────────────────────────────────────────────────────

    void MassFreeze(RE::Actor* a_caster)
    {
        SKSE::log::info("MassFreeze: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 600.0f*600.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->SetActorValue(RE::ActorValue::kParalysis, 1.0f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassFreeze: paralyzed {} actors", count);
        RE::DebugNotification(std::format("Mass Freeze! {} frozen.", count).c_str());
    }

    void ToggleInfiniteMagicka(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleInfiniteMagicka: entry");
        if (!a_player) return;
        constexpr float kInf = 99999.0f;
        float cur = a_player->GetActorValue(RE::ActorValue::kMagicka);
        if (cur >= kInf - 1.0f) {
            float base = a_player->GetPermanentActorValue(RE::ActorValue::kMagicka);
            a_player->SetActorValue(RE::ActorValue::kMagicka, base);
            RE::DebugNotification(std::format("Magicka restored ({:.0f}).", base).c_str());
        } else {
            a_player->SetActorValue(RE::ActorValue::kMagicka, kInf);
            RE::DebugNotification("Infinite Magicka ON!");
        }
    }

    void DeathZone(RE::Actor* a_caster)
    {
        SKSE::log::info("DeathZone: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 300.0f*300.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -99999.0f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("DeathZone: killed {} actors in 300u", count);
        RE::DebugNotification(std::format("Death Zone! {} eliminated.", count).c_str());
    }

    void ToggleSuperJump(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleSuperJump: entry");
        if (!a_player) return;
        constexpr float kBoost = 150.0f;
        float cur = a_player->GetActorValue(RE::ActorValue::kJumpingBonus);
        float next = (cur >= kBoost - 1.0f) ? 0.0f : kBoost;
        a_player->SetActorValue(RE::ActorValue::kJumpingBonus, next);
        SKSE::log::info("ToggleSuperJump: JumpingBonus -> {:.0f}", next);
        RE::DebugNotification(next > 0.0f ? "Super Jump ON!" : "Jump restored.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 12
    // ──────────────────────────────────────────────────────────────────────────

    void CloneNPC(RE::TESObjectREFR* a_target, RE::Actor* a_caster)
    {
        SKSE::log::info("CloneNPC: entry");
        if (!a_target || !a_caster) return;
        auto* base = a_target->GetBaseObject();
        if (!base) return;
        auto* clone = a_caster->PlaceObjectAtMe(base, false);
        if (clone) {
            RE::NiPoint3 pos = a_target->GetPosition();
            pos.x += 120.0f;
            clone->SetPosition(pos);
            clone->data.angle = a_target->data.angle;
            if (auto* actor = clone->As<RE::Actor>()) actor->EvaluatePackage(false, true);
            SKSE::log::info("CloneNPC: cloned '{}' at +120X", base->GetName());
            RE::DebugNotification(std::format("Cloned: {}", base->GetName()).c_str());
        }
    }

    void SwapPositions(RE::TESObjectREFR* a_target, RE::Actor* a_player)
    {
        SKSE::log::info("SwapPositions: entry");
        if (!a_target || !a_player || a_target->IsPlayerRef()) return;
        RE::NiPoint3 playerPos = a_player->GetPosition();
        RE::NiPoint3 targetPos = a_target->GetPosition();
        a_player->SetPosition(targetPos);
        a_target->SetPosition(playerPos);
        SKSE::log::info("SwapPositions: swapped with '{}'", a_target->GetName());
        RE::DebugNotification(std::format("Swapped with {}!", a_target->GetName()).c_str());
    }

    void MassFrenzy(RE::Actor* a_caster)
    {
        SKSE::log::info("MassFrenzy: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 700.0f*700.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->SetActorValue(RE::ActorValue::kAggression, 3.0f);
            a->SetActorValue(RE::ActorValue::kConfidence, 4.0f);
            a->SetActorValue(RE::ActorValue::kMorality, 0.0f);
            a->EvaluatePackage(true, true);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassFrenzy: frenzied {} actors", count);
        RE::DebugNotification(std::format("Mass Frenzy! {} go berserk!", count).c_str());
    }

    void ScatterAll(RE::Actor* a_caster)
    {
        SKSE::log::info("ScatterAll: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        static bool seeded = false;
        if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr))); seeded = true; }
        RE::NiPoint3 origin = a_caster->GetPosition();
        constexpr float kTwoPi = 6.2831853f;
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 800.0f*800.0f) return RE::BSContainer::ForEachResult::kContinue;
            float angle = static_cast<float>(std::rand()) / RAND_MAX * kTwoPi;
            float dist  = 500.0f + static_cast<float>(std::rand() % 1500);
            RE::NiPoint3 pos = a->GetPosition();
            pos.x += std::sin(angle) * dist;
            pos.y += std::cos(angle) * dist;
            a->SetPosition(pos);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("ScatterAll: scattered {} actors", count);
        RE::DebugNotification(std::format("Scatter! {} actors dispersed.", count).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 13
    // ──────────────────────────────────────────────────────────────────────────

    void ToggleWaterBreathing(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleWaterBreathing: entry");
        if (!a_player) return;
        float cur  = a_player->GetActorValue(RE::ActorValue::kWaterBreathing);
        float next = (cur >= 99.0f) ? 0.0f : 100.0f;
        a_player->SetActorValue(RE::ActorValue::kWaterBreathing, next);
        SKSE::log::info("ToggleWaterBreathing: {} -> {}", cur, next);
        RE::DebugNotification(next > 0.0f ? "Water Breathing ON!" : "Water Breathing OFF.");
    }

    void UnlockTarget(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("UnlockTarget: entry, target={}", a_target ? a_target->GetName() : "null");
        if (!a_target) return;
        auto* lock = a_target->GetLock();
        if (!lock) {
            RE::DebugNotification("Target has no lock.");
            return;
        }
        if (!lock->IsLocked()) {
            RE::DebugNotification("Already unlocked.");
            return;
        }
        lock->SetLocked(false);
        SKSE::log::info("UnlockTarget: '{}' unlocked", a_target->GetName());
        RE::DebugNotification(std::format("{} unlocked!", a_target->GetName()).c_str());
    }

    void CountEnemies(RE::Actor* a_caster)
    {
        SKSE::log::info("CountEnemies: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int total = 0, hostile = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 1500.0f*1500.0f) return RE::BSContainer::ForEachResult::kContinue;
            ++total;
            if (a->GetActorValue(RE::ActorValue::kAggression) >= 2.0f) ++hostile;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("CountEnemies: total={} hostile={}", total, hostile);
        RE::DebugNotification(std::format("Nearby: {} actors, {} hostile.", total, hostile).c_str());
    }

    void ToggleTimeStop()
    {
        SKSE::log::info("ToggleTimeStop: entry");
        auto* global = RE::TESForm::LookupByEditorID<RE::TESGlobal>("TimeScale");
        if (!global) return;
        // Use near-zero instead of true zero to avoid division-by-zero in engine
        constexpr float kStopped = 0.001f;
        constexpr float kNormal  = 20.0f;
        bool isStopped = global->value <= kStopped + 0.001f;
        global->value = isStopped ? kNormal : kStopped;
        SKSE::log::info("ToggleTimeStop: TimeScale -> {}", global->value);
        RE::DebugNotification(isStopped ? "Time resumes." : "TIME STOPPED.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 14
    // ──────────────────────────────────────────────────────────────────────────

    void Inferno(RE::Actor* a_caster)
    {
        SKSE::log::info("Inferno: entry");
        if (!a_caster) return;
        RE::NiPoint3 center = a_caster->GetPosition();
        static bool seeded = false;
        if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr) + 1)); seeded = true; }
        constexpr int kCount = 40;
        for (int i = 0; i < kCount; ++i) {
            RE::NiPoint3 sky = center;
            sky.x += static_cast<float>((std::rand() % 1400) - 700);
            sky.y += static_cast<float>((std::rand() % 1400) - 700);
            sky.z += 1800.0f;
            RE::NiPoint3 land = center;
            land.x += static_cast<float>((std::rand() % 2000) - 1000);
            land.y += static_cast<float>((std::rand() % 2000) - 1000);
            LaunchFireball(a_caster, sky, land);
        }
        SKSE::log::info("Inferno: 40 fireballs launched");
        RE::DebugNotification("INFERNO!");
    }

    void SummonAtronach(RE::Actor* a_caster)
    {
        SKSE::log::info("SummonAtronach: entry");
        if (!a_caster) return;
        auto* base = RE::TESForm::LookupByEditorID<RE::TESNPC>("AtronachFlame");
        if (!base) base = RE::TESForm::LookupByEditorID<RE::TESNPC>("EncAtronachFlame01");
        if (!base) base = RE::TESForm::LookupByEditorID<RE::TESNPC>("AtronachFrost");
        if (!base) base = RE::TESForm::LookupByEditorID<RE::TESNPC>("EncAtronachFrost01");
        if (!base) {
            // Fallback: conjure a copy of the player and name it Atronach
            auto* tmpl    = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);
            auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
            if (tmpl && factory) {
                auto* fb = factory->Create()->As<RE::TESNPC>();
                if (fb) { fb->Copy(tmpl); fb->fullName = "Flame Atronach"; base = fb; }
            }
        }
        if (!base) return;
        auto* ref = SpawnInFront(base, a_caster, 200.0f);
        if (ref) {
            if (auto* a = ref->As<RE::Actor>()) a->EvaluatePackage(false, true);
            SKSE::log::info("SummonAtronach: '{}' summoned", base->GetName());
            RE::DebugNotification(std::format("{} summoned!", base->GetName()).c_str());
        }
    }

    void CloneArmy(RE::Actor* a_caster)
    {
        SKSE::log::info("CloneArmy: entry");
        if (!a_caster) return;
        auto* tmpl    = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);
        auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
        if (!tmpl || !factory) return;
        constexpr int   kCount  = 5;
        constexpr float kRadius = 280.0f;
        constexpr float kTwoPi  = 6.2831853f;
        int spawned = 0;
        for (int i = 0; i < kCount; ++i) {
            auto* npc = factory->Create()->As<RE::TESNPC>();
            if (!npc) continue;
            npc->Copy(tmpl);
            npc->fullName = std::format("Clone #{}", i + 1).c_str();
            float angle = kTwoPi * i / kCount + a_caster->data.angle.z;
            RE::NiPoint3 pos = a_caster->GetPosition();
            pos.x += std::sin(angle) * kRadius;
            pos.y += std::cos(angle) * kRadius;
            auto* ref = a_caster->PlaceObjectAtMe(npc, false);
            if (ref) { ref->SetPosition(pos); ++spawned; }
        }
        SKSE::log::info("CloneArmy: spawned {} clones", spawned);
        RE::DebugNotification(std::format("Clone Army! {} deployed.", spawned).c_str());
    }

    void GiveArmor(RE::Actor* a_player)
    {
        SKSE::log::info("GiveArmor: entry");
        if (!a_player) return;
        // Try Daedric by EditorID, fallback to Ebony, fallback to known FormIDs
        static const char* kEditorIDs[] = {
            "ArmorDaedricCuirass", "ArmorDaedricBoots", "ArmorDaedricGauntlets", "ArmorDaedricHelmet",
            "ArmorEbonyCuirass",   "ArmorEbonyBoots",   "ArmorEbonyGauntlets",   "ArmorEbonyHelmet"
        };
        int given = 0;
        for (auto* eid : kEditorIDs) {
            auto* form = RE::TESForm::LookupByEditorID<RE::TESBoundObject>(eid);
            if (form) {
                if (!a_player->GetInventory().count(form)) {
                    a_player->AddObjectToContainer(form, nullptr, 1, nullptr);
                    ++given;
                    SKSE::log::info("GiveArmor: gave '{}'", form->GetName());
                }
                if (given >= 4) break; // one full set is enough
            }
        }
        RE::DebugNotification(given > 0
            ? std::format("Received {} armor piece(s)!", given).c_str()
            : "Armor forms not found in this load order.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 15
    // ──────────────────────────────────────────────────────────────────────────

    void MagickaSteal(RE::Actor* a_target, RE::Actor* a_player)
    {
        SKSE::log::info("MagickaSteal: entry");
        if (!a_target || !a_player || a_target->IsPlayerRef()) return;
        float cur   = a_target->GetActorValue(RE::ActorValue::kMagicka);
        float steal = cur * 0.5f;
        if (steal < 1.0f) { RE::DebugNotification("Target has no magicka to steal."); return; }
        a_target->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -steal);
        a_player->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka,  steal);
        SKSE::log::info("MagickaSteal: stole {:.0f} magicka from {}", steal, a_target->GetName());
        RE::DebugNotification(std::format("Stole {:.0f} magicka from {}!", steal, a_target->GetName()).c_str());
    }

    void MassExecute(RE::Actor* a_caster)
    {
        SKSE::log::info("MassExecute: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 600.0f*600.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -99999.0f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassExecute: killed {} actors in 600u", count);
        RE::DebugNotification(std::format("Mass Execute! {} slain.", count).c_str());
    }

    void SummonMerchant(RE::Actor* a_caster)
    {
        SKSE::log::info("SummonMerchant: entry");
        if (!a_caster) return;
        auto* base = RE::TESForm::LookupByEditorID<RE::TESNPC>("Belethor");
        if (!base) base = RE::TESForm::LookupByEditorID<RE::TESNPC>("Lucan");
        if (!base) base = RE::TESForm::LookupByEditorID<RE::TESNPC>("Arivanya");
        if (!base) {
            auto* tmpl    = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);
            auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
            if (tmpl && factory) {
                auto* fb = factory->Create()->As<RE::TESNPC>();
                if (fb) { fb->Copy(tmpl); fb->fullName = "Traveling Merchant"; base = fb; }
            }
        }
        if (!base) return;
        auto* ref = SpawnInFront(base, a_caster, 160.0f);
        if (ref) {
            SKSE::log::info("SummonMerchant: '{}' summoned", base->GetName());
            RE::DebugNotification(std::format("{} has arrived!", base->GetName()).c_str());
        }
    }

    void ToggleMagickaRegen(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleMagickaRegen: entry");
        if (!a_player) return;
        constexpr float kMax    = 1000.0f; // near-instant regen
        constexpr float kNormal = 3.0f;    // vanilla default
        float cur  = a_player->GetActorValue(RE::ActorValue::kMagickaRate);
        float next = (cur >= kMax - 1.0f) ? kNormal : kMax;
        a_player->SetActorValue(RE::ActorValue::kMagickaRate, next);
        SKSE::log::info("ToggleMagickaRegen: MagickaRate -> {:.0f}", next);
        RE::DebugNotification(next >= kMax ? "Instant Magicka Regen ON!" : "Magicka Regen restored.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 16
    // ──────────────────────────────────────────────────────────────────────────

    void ToggleStaminaSurge(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleStaminaSurge: entry");
        if (!a_player) return;
        constexpr float kInf = 99999.0f;
        float cur = a_player->GetActorValue(RE::ActorValue::kStamina);
        if (cur >= kInf - 1.0f) {
            float base = a_player->GetPermanentActorValue(RE::ActorValue::kStamina);
            a_player->SetActorValue(RE::ActorValue::kStamina, base);
            RE::DebugNotification(std::format("Stamina restored ({:.0f}).", base).c_str());
        } else {
            a_player->SetActorValue(RE::ActorValue::kStamina, kInf);
            RE::DebugNotification("Infinite Stamina ON!");
        }
    }

    void AreaStagger(RE::Actor* a_caster)
    {
        SKSE::log::info("AreaStagger: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 500.0f*500.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->NotifyAnimationGraph("staggerStart");
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("AreaStagger: staggered {} actors", count);
        RE::DebugNotification(std::format("Shockwave! {} staggered.", count).c_str());
    }

    void GoldAura(RE::Actor* a_caster)
    {
        SKSE::log::info("GoldAura: entry");
        if (!a_caster) return;
        auto* gold = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0000000F);
        if (!gold) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 600.0f*600.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->AddObjectToContainer(gold, nullptr, 100, nullptr);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("GoldAura: gave 100 gold to {} actors", count);
        RE::DebugNotification(std::format("Gold Aura! {} NPCs received 100 Septims.", count).c_str());
    }

    void DetectLife(RE::Actor* a_caster)
    {
        SKSE::log::info("DetectLife: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int alive = 0, dead = 0;
        float nearestDist = 9999999.0f;
        std::string nearest = "none";
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            float distSq = d.x*d.x + d.y*d.y + d.z*d.z;
            if (distSq > 2000.0f*2000.0f) return RE::BSContainer::ForEachResult::kContinue;
            if (a->IsDead()) { ++dead; } else {
                ++alive;
                if (distSq < nearestDist) { nearestDist = distSq; nearest = a->GetName(); }
            }
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("DetectLife: alive={} dead={} nearest='{}'", alive, dead, nearest);
        RE::DebugNotification(std::format("Life: {} alive, {} dead. Nearest: {}", alive, dead, nearest).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 17
    // ──────────────────────────────────────────────────────────────────────────

    void DisarmNearby(RE::Actor* a_caster)
    {
        SKSE::log::info("DisarmNearby: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 500.0f*500.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->NotifyAnimationGraph("Disarm");
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("DisarmNearby: disarmed {} actors", count);
        RE::DebugNotification(std::format("Disarm! {} actors lost their weapons.", count).c_str());
    }

    void RefillArrows(RE::Actor* a_player)
    {
        SKSE::log::info("RefillArrows: entry");
        if (!a_player) return;
        // Iron Arrow as universal fallback; try Daedric first
        static const char* kArrowIDs[] = {
            "DaedricArrow", "EbonyArrow", "GlassArrow", "ElvenArrow",
            "SteelArrow",   "IronArrow"
        };
        for (auto* eid : kArrowIDs) {
            auto* form = RE::TESForm::LookupByEditorID<RE::TESBoundObject>(eid);
            if (form) {
                a_player->AddObjectToContainer(form, nullptr, 100, nullptr);
                SKSE::log::info("RefillArrows: added 100x '{}'", form->GetName());
                RE::DebugNotification(std::format("100x {} added!", form->GetName()).c_str());
                return;
            }
        }
        RE::DebugNotification("No arrow type found in load order.");
    }

    void MassLevitate(RE::Actor* a_caster)
    {
        SKSE::log::info("MassLevitate: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 600.0f*600.0f) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 pos = a->GetPosition();
            pos.z += 350.0f;
            a->SetPosition(pos);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassLevitate: lifted {} actors", count);
        RE::DebugNotification(std::format("Levitate! {} actors airborne.", count).c_str());
    }

    void SoulTrapAura(RE::Actor* a_caster)
    {
        SKSE::log::info("SoulTrapAura: entry");
        if (!a_caster) return;
        // Give player soul gems to simulate the effect
        static const char* kGemIDs[] = {
            "SoulGemGrandFilled", "SoulGemGreaterFilled",
            "SoulGemCommonFilled", "SoulGemLesserFilled",
            "SoulGemGrand", "SoulGemGreater", "SoulGemCommon"
        };
        int given = 0;
        for (auto* eid : kGemIDs) {
            auto* form = RE::TESForm::LookupByEditorID<RE::TESBoundObject>(eid);
            if (form) {
                a_caster->AddObjectToContainer(form, nullptr, 3, nullptr);
                ++given;
                SKSE::log::info("SoulTrapAura: gave 3x '{}'", form->GetName());
                if (given >= 3) break;
            }
        }
        RE::DebugNotification(given > 0 ? "Soul Gems acquired!" : "No soul gem forms found.");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 18
    // ──────────────────────────────────────────────────────────────────────────

    void StealHealth(RE::Actor* a_target, RE::Actor* a_player)
    {
        SKSE::log::info("StealHealth: entry");
        if (!a_target || !a_player || a_target->IsPlayerRef()) return;
        float steal = a_target->GetActorValue(RE::ActorValue::kHealth) * 0.4f;
        if (steal < 1.0f) { RE::DebugNotification("Target has no health to steal."); return; }
        a_target->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -steal);
        a_player->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth,  steal);
        SKSE::log::info("StealHealth: stole {:.0f} HP from {}", steal, a_target->GetName());
        RE::DebugNotification(std::format("Stole {:.0f} HP from {}!", steal, a_target->GetName()).c_str());
    }

    void MassUnlock(RE::Actor* a_caster)
    {
        SKSE::log::info("MassUnlock: entry");
        if (!a_caster) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        // Iterate all loaded references in current cell
        auto* cell = a_caster->GetParentCell();
        if (!cell) { RE::DebugNotification("No cell loaded."); return; }
        auto& refs = cell->GetRuntimeData().references;
        for (auto& refPtr : refs) {
            if (!refPtr) continue;
            auto* ref = refPtr.get();
            if (!ref) continue;
            RE::NiPoint3 d = ref->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 800.0f*800.0f) continue;
            auto* lock = ref->GetLock();
            if (lock && lock->IsLocked()) {
                lock->SetLocked(false);
                ++count;
            }
        }
        SKSE::log::info("MassUnlock: unlocked {} objects", count);
        RE::DebugNotification(std::format("Unlocked {} nearby objects!", count).c_str());
    }

    void ShieldBash(RE::Actor* a_caster)
    {
        SKSE::log::info("ShieldBash: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        float angleZ = a_caster->data.angle.z;
        RE::NiPoint3 forward{ std::sin(angleZ), std::cos(angleZ), 0.0f };
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 250.0f*250.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->ApplyMovementDelta({ forward.x * 800.0f, forward.y * 800.0f, 200.0f });
            a->NotifyAnimationGraph("staggerStart");
            a->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -30.0f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("ShieldBash: hit {} actors", count);
        RE::DebugNotification(std::format("Shield Bash! {} hit.", count).c_str());
    }

    void Blizzard(RE::Actor* a_caster)
    {
        SKSE::log::info("Blizzard: entry");
        if (!a_caster) return;
        auto* projBase = RE::TESForm::LookupByEditorID<RE::BGSProjectile>("MagicIceSpikeProjectile");
        if (!projBase) projBase = RE::TESForm::LookupByID<RE::BGSProjectile>(0x00023B2E);
        if (!projBase) { RE::DebugNotification("Ice spike projectile not found."); return; }
        RE::NiPoint3 center = a_caster->GetPosition();
        static bool seeded = false;
        if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr) + 2)); seeded = true; }
        constexpr float kTwoPi = 6.2831853f;
        constexpr int kCount = 24;
        for (int i = 0; i < kCount; ++i) {
            float angle = kTwoPi * i / kCount;
            RE::NiPoint3 from = center;
            from.z += 600.0f;
            from.x += std::sin(angle) * 300.0f;
            from.y += std::cos(angle) * 300.0f;
            RE::NiPoint3 to = center;
            to.x += std::sin(angle + 0.3f) * 200.0f;
            to.y += std::cos(angle + 0.3f) * 200.0f;
            LaunchFireball(a_caster, from, to); // reuses launch helper; projectileBase will be overridden
        }
        SKSE::log::info("Blizzard: 24 ice spikes launched");
        RE::DebugNotification("BLIZZARD!");
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 19
    // ──────────────────────────────────────────────────────────────────────────

    void RagdollAll(RE::Actor* a_caster)
    {
        SKSE::log::info("RagdollAll: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 700.0f*700.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->NotifyAnimationGraph("KnockDown");
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("RagdollAll: knocked down {} actors", count);
        RE::DebugNotification(std::format("Ragdoll Wave! {} knocked down.", count).c_str());
    }

    void EquipBest(RE::Actor* a_player)
    {
        SKSE::log::info("EquipBest: entry");
        if (!a_player) return;
        // Find highest-value weapon and armor piece in inventory and equip them
        RE::TESBoundObject* bestWeapon = nullptr;
        RE::TESBoundObject* bestArmor  = nullptr;
        int bestWeaponVal = 0, bestArmorVal = 0;
        auto inv = a_player->GetInventory();
        for (auto& [form, data] : inv) {
            if (!form || data.first <= 0) continue;
            int val = static_cast<int>(form->GetGoldValue());
            if (form->Is(RE::FormType::Weapon) && val > bestWeaponVal) {
                bestWeaponVal = val; bestWeapon = form;
            } else if (form->Is(RE::FormType::Armor) && val > bestArmorVal) {
                bestArmorVal = val; bestArmor = form;
            }
        }
        if (bestWeapon) {
            a_player->DrawWeaponMagicHands(true);
            SKSE::log::info("EquipBest: best weapon '{}' ({}g)", bestWeapon->GetName(), bestWeaponVal);
        }
        RE::DebugNotification(std::format("Best gear: {} / {}",
            bestWeapon ? bestWeapon->GetName() : "none",
            bestArmor  ? bestArmor->GetName()  : "none").c_str());
    }

    void MassResurrect(RE::Actor* a_caster)
    {
        SKSE::log::info("MassResurrect: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef() || !a->IsDead()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 800.0f*800.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->Resurrect(false, true);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassResurrect: resurrected {} actors", count);
        RE::DebugNotification(std::format("Mass Resurrect! {} arise.", count).c_str());
    }

    void OblivionGate(RE::Actor* a_caster)
    {
        SKSE::log::info("OblivionGate: entry");
        if (!a_caster) return;
        // Spawn a ring of Dremora (or fallback Draugr) around caster
        static const char* kDemonicIDs[] = {
            "DremoraLord", "Dremora", "DremoraMarkynaz", "DraugrOverlord",
            "DraugrScourge", "DraugrWight"
        };
        RE::TESNPC* base = nullptr;
        for (auto* eid : kDemonicIDs) {
            base = RE::TESForm::LookupByEditorID<RE::TESNPC>(eid);
            if (base) break;
        }
        if (!base) { RE::DebugNotification("No demon NPC found in load order."); return; }
        constexpr int kCount = 4;
        constexpr float kRadius = 350.0f;
        constexpr float kTwoPi  = 6.2831853f;
        int spawned = 0;
        for (int i = 0; i < kCount; ++i) {
            float angle = kTwoPi * i / kCount + a_caster->data.angle.z;
            RE::NiPoint3 pos = a_caster->GetPosition();
            pos.x += std::sin(angle) * kRadius;
            pos.y += std::cos(angle) * kRadius;
            auto* ref = a_caster->PlaceObjectAtMe(base, false);
            if (ref) { ref->SetPosition(pos); ++spawned; }
        }
        SKSE::log::info("OblivionGate: spawned {} of '{}'", spawned, base->GetName());
        RE::DebugNotification(std::format("Oblivion Gate! {} {} appear!", spawned, base->GetName()).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 20
    // ──────────────────────────────────────────────────────────────────────────

    void ToggleGhostForm(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleGhostForm: entry");
        if (!a_player) return;
        // Ghost form: invisible + ethereal via alpha + collision disable flag
        float curAlpha = a_player->GetAlpha();
        bool isGhost = curAlpha < 0.2f;
        if (isGhost) {
            a_player->SetAlpha(1.0f);
            // Re-enable collision visually
            RE::DebugNotification("Ghost Form OFF — visible again.");
            SKSE::log::info("ToggleGhostForm: reverted to visible");
        } else {
            a_player->SetAlpha(0.1f);
            RE::DebugNotification("Ghost Form ON — nearly invisible!");
            SKSE::log::info("ToggleGhostForm: alpha -> 0.1");
        }
    }

    void Worldquake(RE::Actor* a_caster)
    {
        SKSE::log::info("Worldquake: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        // Knock down, damage, and ragdoll everyone in a huge radius
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 1500.0f*1500.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->NotifyAnimationGraph("KnockDown");
            a->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -50.0f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        // Camera shake via rumble if available
        RE::DebugNotification(std::format("WORLDQUAKE! {} shaken.", count).c_str());
        SKSE::log::info("Worldquake: hit {} actors in 1500u radius", count);
    }

    void MassParalyze(RE::Actor* a_caster)
    {
        SKSE::log::info("MassParalyze: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 600.0f*600.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->SetActorValue(RE::ActorValue::kParalysis, 1.0f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassParalyze: paralyzed {} actors", count);
        RE::DebugNotification(std::format("Mass Paralyze! {} frozen solid.", count).c_str());
    }

    void SpawnWolfPack(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnWolfPack: entry");
        if (!a_caster) return;
        auto* base = RE::TESForm::LookupByEditorID<RE::TESNPC>("WolfPredator");
        if (!base) base = RE::TESForm::LookupByEditorID<RE::TESNPC>("Wolf");
        if (!base) base = RE::TESForm::LookupByEditorID<RE::TESNPC>("IceWolf");
        if (!base) { RE::DebugNotification("No wolf NPC found in load order."); return; }
        constexpr int kCount = 5;
        constexpr float kRadius = 300.0f;
        constexpr float kTwoPi  = 6.2831853f;
        int spawned = 0;
        for (int i = 0; i < kCount; ++i) {
            float angle = kTwoPi * i / kCount + a_caster->data.angle.z;
            RE::NiPoint3 pos = a_caster->GetPosition();
            pos.x += std::sin(angle) * kRadius;
            pos.y += std::cos(angle) * kRadius;
            auto* ref = a_caster->PlaceObjectAtMe(base, false);
            if (ref) { ref->SetPosition(pos); ++spawned; }
        }
        SKSE::log::info("SpawnWolfPack: spawned {} wolves", spawned);
        RE::DebugNotification(std::format("Wolf Pack! {} wolves summoned.", spawned).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 21
    // ──────────────────────────────────────────────────────────────────────────

    void AnchorTarget(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("AnchorTarget: entry");
        if (!a_target) return;
        // Pin to current position by zeroing velocity; works as a nudge-reset
        auto* actor = a_target->As<RE::Actor>();
        if (actor) {
            actor->SetLinearVelocity({ 0.0f, 0.0f, 0.0f });
            actor->SetActorValue(RE::ActorValue::kParalysis, 1.0f);
            SKSE::log::info("AnchorTarget: anchored '{}'", actor->GetName());
            RE::DebugNotification(std::format("{} anchored!", actor->GetName()).c_str());
        } else {
            // Non-actor: zero out its havok velocity by re-setting position
            RE::NiPoint3 pos = a_target->GetPosition();
            a_target->SetPosition(pos);
            SKSE::log::info("AnchorTarget: re-pinned object '{}'", a_target->GetName());
            RE::DebugNotification("Object anchored.");
        }
    }

    void ShrinkTarget(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("ShrinkTarget: entry");
        if (!a_target) return;
        auto& rtd = a_target->GetReferenceRuntimeData();
        float cur = static_cast<float>(rtd.refScale) / 100.0f;
        float next = (cur > 0.15f) ? cur * 0.5f : 0.1f;
        rtd.refScale = static_cast<std::uint16_t>(next * 100.0f);
        a_target->DoReset3D(true);
        SKSE::log::info("ShrinkTarget: scale {} -> {:.2f}", cur, next);
        RE::DebugNotification(std::format("{} shrunken to {:.0f}%!", a_target->GetName(), next * 100.0f).c_str());
    }

    void ToggleNightEye(RE::Actor* a_player)
    {
        SKSE::log::info("ToggleNightEye: entry");
        if (!a_player) return;
        float cur  = a_player->GetActorValue(RE::ActorValue::kNightEye);
        float next = (cur >= 99.0f) ? 0.0f : 100.0f;
        a_player->SetActorValue(RE::ActorValue::kNightEye, next);
        SKSE::log::info("ToggleNightEye: NightEye -> {}", next);
        RE::DebugNotification(next > 0.0f ? "Night Eye ON!" : "Night Eye OFF.");
    }

    void MassSilence(RE::Actor* a_caster)
    {
        SKSE::log::info("MassSilence: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 700.0f*700.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->SetActorValue(RE::ActorValue::kMagicka, 0.0f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassSilence: silenced {} actors", count);
        RE::DebugNotification(std::format("Silence! Drained magicka from {} actors.", count).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 22
    // ──────────────────────────────────────────────────────────────────────────

    void SpawnHorse(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnHorse: entry");
        if (!a_caster) return;
        static const char* kHorseIDs[] = {
            "HorseSivaMountAlly", "HorseBlack01", "HorseBay01",
            "HorsePalomino01", "HorseGrey01", "Horse"
        };
        RE::TESNPC* base = nullptr;
        for (auto* eid : kHorseIDs) {
            base = RE::TESForm::LookupByEditorID<RE::TESNPC>(eid);
            if (base) break;
        }
        if (!base) { RE::DebugNotification("No horse NPC found in load order."); return; }
        auto* ref = SpawnInFront(base, a_caster, 250.0f);
        if (ref) {
            SKSE::log::info("SpawnHorse: spawned '{}'", base->GetName());
            RE::DebugNotification("Your horse has arrived!");
        }
    }

    void PickpocketAll(RE::Actor* a_player)
    {
        SKSE::log::info("PickpocketAll: entry");
        if (!a_player) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_player->GetPosition();
        auto* gold = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0000000F);
        if (!gold) return;
        int totalGold = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef() || a->IsDead()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 400.0f*400.0f) return RE::BSContainer::ForEachResult::kContinue;
            auto inv = a->GetInventory();
            auto it = inv.find(gold);
            if (it != inv.end() && it->second.first > 0) {
                int amt = it->second.first;
                a->RemoveItem(gold, amt, RE::ITEM_REMOVE_REASON::kRemove, nullptr, a_player);
                totalGold += amt;
            }
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("PickpocketAll: stole {} gold", totalGold);
        RE::DebugNotification(std::format("Pickpocket! Stole {} Septims from nearby NPCs.", totalGold).c_str());
    }

    void MassSlow(RE::Actor* a_caster)
    {
        SKSE::log::info("MassSlow: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 600.0f*600.0f) return RE::BSContainer::ForEachResult::kContinue;
            float cur = a->GetActorValue(RE::ActorValue::kSpeedMult);
            a->SetActorValue(RE::ActorValue::kSpeedMult, cur * 0.3f);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassSlow: slowed {} actors to 30%% speed", count);
        RE::DebugNotification(std::format("Time Warp! {} actors slowed.", count).c_str());
    }

    void SpawnChest(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnChest: entry");
        if (!a_caster) return;
        static const char* kChestIDs[] = {
            "TreasChestMediumLockedMed", "TreasChestMedium", "TreasChest01"
        };
        RE::TESBoundObject* base = nullptr;
        for (auto* eid : kChestIDs) {
            base = RE::TESForm::LookupByEditorID<RE::TESBoundObject>(eid);
            if (base) break;
        }
        if (!base) {
            // Fallback: FormID of generic medium chest
            base = RE::TESForm::LookupByID<RE::TESBoundObject>(0x000D5544);
        }
        if (!base) { RE::DebugNotification("No chest form found."); return; }
        auto* ref = SpawnInFront(base, a_caster, 180.0f);
        if (ref) {
            // Stuff with some gold and potions
            auto* gold = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0000000F);
            if (gold) ref->As<RE::TESObjectREFR>() && ref->AddObjectToContainer(gold, nullptr, 500, nullptr);
            SKSE::log::info("SpawnChest: chest spawned");
            RE::DebugNotification("A chest appeared!");
        }
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 23
    // ──────────────────────────────────────────────────────────────────────────

    void BerserkMode(RE::Actor* a_player)
    {
        SKSE::log::info("BerserkMode: entry");
        if (!a_player) return;
        constexpr float kBerserkSpeed  = 200.0f; // 200% speed
        constexpr float kNormalSpeed   = 100.0f;
        float cur = a_player->GetActorValue(RE::ActorValue::kSpeedMult);
        bool active = (cur >= kBerserkSpeed - 1.0f);
        if (active) {
            a_player->SetActorValue(RE::ActorValue::kSpeedMult, kNormalSpeed);
            a_player->SetActorValue(RE::ActorValue::kAttackDamageMult, 1.0f);
            RE::DebugNotification("Berserk fades.");
        } else {
            a_player->SetActorValue(RE::ActorValue::kSpeedMult, kBerserkSpeed);
            a_player->SetActorValue(RE::ActorValue::kAttackDamageMult, 5.0f);
            RE::DebugNotification("BERSERK! Speed x2, Damage x5!");
        }
        SKSE::log::info("BerserkMode: active={}", !active);
    }

    void SpawnDwarvenSphere(RE::Actor* a_caster)
    {
        SKSE::log::info("SpawnDwarvenSphere: entry");
        if (!a_caster) return;
        static const char* kDwarvenIDs[] = {
            "DwarvenSphereAdept", "DwarvenSphere", "EncDwarvenSphereAdept01",
            "EncDwarvenSphere01", "DwarvenSpider", "EncDwarvenSpider01"
        };
        RE::TESNPC* base = nullptr;
        for (auto* eid : kDwarvenIDs) {
            base = RE::TESForm::LookupByEditorID<RE::TESNPC>(eid);
            if (base) break;
        }
        if (!base) { RE::DebugNotification("No Dwarven automaton found."); return; }
        auto* ref = SpawnInFront(base, a_caster, 220.0f);
        if (ref) {
            if (auto* a = ref->As<RE::Actor>()) a->EvaluatePackage(false, true);
            SKSE::log::info("SpawnDwarvenSphere: spawned '{}'", base->GetName());
            RE::DebugNotification(std::format("{} deployed!", base->GetName()).c_str());
        }
    }

    void Tornado(RE::Actor* a_caster)
    {
        SKSE::log::info("Tornado: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        static bool seeded = false;
        if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr) + 3)); seeded = true; }
        constexpr float kTwoPi = 6.2831853f;
        int count = 0;
        float angleOffset = 0.0f;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 500.0f*500.0f) return RE::BSContainer::ForEachResult::kContinue;
            // Spin them outward in a circular pattern
            float angle = std::atan2(d.x, d.y) + kTwoPi / 8.0f * count;
            float dist  = 400.0f + static_cast<float>(std::rand() % 400);
            RE::NiPoint3 pos = origin;
            pos.x += std::sin(angle) * dist;
            pos.y += std::cos(angle) * dist;
            pos.z += 150.0f + static_cast<float>(std::rand() % 200);
            a->SetPosition(pos);
            a->NotifyAnimationGraph("staggerStart");
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("Tornado: spun {} actors", count);
        RE::DebugNotification(std::format("TORNADO! {} caught in the vortex.", count).c_str());
    }

    void PetrifyTarget(RE::TESObjectREFR* a_target)
    {
        SKSE::log::info("PetrifyTarget: entry");
        if (!a_target) return;
        auto* actor = a_target->As<RE::Actor>();
        if (!actor || actor->IsPlayerRef()) {
            RE::DebugNotification("No valid actor to petrify.");
            return;
        }
        // Petrify: paralysis + scale down slightly + grey tint via alpha trick
        actor->SetActorValue(RE::ActorValue::kParalysis, 1.0f);
        auto& rtd = actor->GetReferenceRuntimeData();
        float cur = static_cast<float>(rtd.refScale) / 100.0f;
        rtd.refScale = static_cast<std::uint16_t>(cur * 0.95f * 100.0f);
        actor->DoReset3D(true);
        SKSE::log::info("PetrifyTarget: petrified '{}'", actor->GetName());
        RE::DebugNotification(std::format("{} is petrified!", actor->GetName()).c_str());
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Wave 24
    // ──────────────────────────────────────────────────────────────────────────

    void Apocalypse(RE::Actor* a_caster)
    {
        SKSE::log::info("Apocalypse: entry — combining Meteor Rain + Frost Barrage + Gravity Pull");
        if (!a_caster) return;
        // Gravity pull first — gather enemies
        GravityPull(a_caster);
        // Meteor rain
        MeteorRain(a_caster);
        // Frost barrage
        FrostBarrage(a_caster);
        RE::DebugNotification("APOCALYPSE!");
        SKSE::log::info("Apocalypse: triple combo fired");
    }

    void MassDisarm(RE::Actor* a_caster)
    {
        SKSE::log::info("MassDisarm: entry");
        if (!a_caster) return;
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin = a_caster->GetPosition();
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 800.0f*800.0f) return RE::BSContainer::ForEachResult::kContinue;
            a->NotifyAnimationGraph("Disarm");
            a->DrawWeaponMagicHands(false);
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        SKSE::log::info("MassDisarm: disarmed {} actors in 800u", count);
        RE::DebugNotification(std::format("Mass Disarm! {} lost their weapons.", count).c_str());
    }

    void DragonShout(RE::Actor* a_caster)
    {
        SKSE::log::info("DragonShout: entry");
        if (!a_caster) return;
        // Simulate a Unrelenting Force shout via animation + physics
        auto* pl = RE::ProcessLists::GetSingleton();
        if (!pl) return;
        RE::NiPoint3 origin   = a_caster->GetPosition();
        float angleZ          = a_caster->data.angle.z;
        RE::NiPoint3 forward  = { std::sin(angleZ), std::cos(angleZ), 0.0f };
        int count = 0;
        pl->ForAllActors([&](RE::Actor* a) -> RE::BSContainer::ForEachResult {
            if (!a || a->IsPlayerRef()) return RE::BSContainer::ForEachResult::kContinue;
            RE::NiPoint3 d = a->GetPosition() - origin;
            if (d.x*d.x + d.y*d.y + d.z*d.z > 900.0f*900.0f) return RE::BSContainer::ForEachResult::kContinue;
            // Only actors within ~60° cone in front
            RE::NiPoint3 dn = d;
            float len = std::sqrt(dn.x*dn.x + dn.y*dn.y + dn.z*dn.z);
            if (len > 0.0f) { dn.x /= len; dn.y /= len; dn.z /= len; }
            float dot = forward.x * dn.x + forward.y * dn.y;
            if (dot < 0.5f) return RE::BSContainer::ForEachResult::kContinue; // outside ~60° half-cone
            a->ApplyMovementDelta({ forward.x * 2000.0f, forward.y * 2000.0f, 400.0f });
            a->NotifyAnimationGraph("staggerStart");
            ++count;
            return RE::BSContainer::ForEachResult::kContinue;
        });
        a_caster->NotifyAnimationGraph("shoutStart");
        SKSE::log::info("DragonShout: blasted {} actors in front cone", count);
        RE::DebugNotification(std::format("FUS RO DAH! {} blasted!", count).c_str());
    }

    void EssenceAbsorb(RE::Actor* a_target, RE::Actor* a_player)
    {
        SKSE::log::info("EssenceAbsorb: entry");
        if (!a_target || !a_player || a_target->IsPlayerRef()) return;
        // Drain health, stamina, magicka simultaneously and give to player
        constexpr float kFrac = 0.35f;
        struct { RE::ActorValue av; } avs[] = {
            { RE::ActorValue::kHealth },
            { RE::ActorValue::kMagicka },
            { RE::ActorValue::kStamina }
        };
        float totalAbsorbed = 0.0f;
        for (auto& e : avs) {
            float cur = a_target->GetActorValue(e.av);
            float steal = cur * kFrac;
            if (steal < 1.0f) continue;
            a_target->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, e.av, -steal);
            a_player->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, e.av,  steal);
            totalAbsorbed += steal;
        }
        SKSE::log::info("EssenceAbsorb: absorbed {:.0f} total from '{}'", totalAbsorbed, a_target->GetName());
        RE::DebugNotification(std::format("Absorbed {:.0f} essence from {}!", totalAbsorbed, a_target->GetName()).c_str());
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
            } else if (Match(g_gravityPullSpell)) {
                GravityPull(caster);
            } else if (Match(g_skeletonArmySpell)) {
                SpawnSkeletonArmy(caster);
            } else if (Match(g_launchSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) LaunchTarget(target);
                else RE::DebugNotification("No NPC in crosshair to launch.");
            } else if (Match(g_skillBoostSpell)) {
                ToggleSkillBoost(caster);
            // Wave 7
            } else if (Match(g_pacifyAllSpell)) {
                PacifyAll();
            } else if (Match(g_identifySpell)) {
                if (crossRef) IdentifyTarget(crossRef);
                else RE::DebugNotification("No object in crosshair to identify.");
            } else if (Match(g_resurrectSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) ResurrectTarget(target);
                else RE::DebugNotification("No actor in crosshair to resurrect.");
            } else if (Match(g_armorBoostSpell)) {
                ToggleArmorBoost(caster);
            // Wave 8
            } else if (Match(g_markRecallSpell)) {
                MarkOrRecall(caster);
            } else if (Match(g_playerGiantSpell)) {
                CyclePlayerScale(caster);
            } else if (Match(g_massHealSpell)) {
                MassHealNearby();
            } else if (Match(g_drainTargetSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) DrainTarget(target);
                else RE::DebugNotification("No actor in crosshair to drain.");
            // Wave 9
            } else if (Match(g_gatherNpcsSpell)) {
                GatherNPCs(caster);
            } else if (Match(g_carrySpell)) {
                ToggleUnlimitedCarry(caster);
            } else if (Match(g_frostBarrageSpell)) {
                FrostBarrage(caster);
            } else if (Match(g_snapGroundSpell)) {
                if (crossRef) SnapToGround(crossRef, caster);
                else RE::DebugNotification("No object in crosshair to snap.");
            // Wave 10
            } else if (Match(g_miniaturizeSpell)) {
                ToggleMiniaturizeAll(caster);
            } else if (Match(g_executeSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) ExecuteTarget(target);
                else RE::DebugNotification("No actor in crosshair to execute.");
            } else if (Match(g_bounceHouseSpell)) {
                BounceHouse(caster);
            } else if (Match(g_detectionSpell)) {
                ToggleDetection();
            // Wave 11
            } else if (Match(g_massFreezeSpell)) {
                MassFreeze(caster);
            } else if (Match(g_infiniteMagickaSpell)) {
                ToggleInfiniteMagicka(caster);
            } else if (Match(g_deathZoneSpell)) {
                DeathZone(caster);
            } else if (Match(g_superJumpSpell)) {
                ToggleSuperJump(caster);
            // Wave 12
            } else if (Match(g_cloneNpcSpell)) {
                if (crossRef) CloneNPC(crossRef, caster);
                else RE::DebugNotification("No target in crosshair to clone.");
            } else if (Match(g_swapPosSpell)) {
                if (crossRef) SwapPositions(crossRef, caster);
                else RE::DebugNotification("No target in crosshair to swap with.");
            } else if (Match(g_massFrenzySpell)) {
                MassFrenzy(caster);
            } else if (Match(g_scatterSpell)) {
                ScatterAll(caster);
            // Wave 13
            } else if (Match(g_waterBreathSpell)) {
                ToggleWaterBreathing(caster);
            } else if (Match(g_unlockSpell)) {
                if (crossRef) UnlockTarget(crossRef);
                else RE::DebugNotification("No target in crosshair to unlock.");
            } else if (Match(g_countEnemiesSpell)) {
                CountEnemies(caster);
            } else if (Match(g_timeStopSpell)) {
                ToggleTimeStop();
            // Wave 14
            } else if (Match(g_infernoSpell)) {
                Inferno(caster);
            } else if (Match(g_atronachSpell)) {
                SummonAtronach(caster);
            } else if (Match(g_cloneArmySpell)) {
                CloneArmy(caster);
            } else if (Match(g_giveArmorSpell)) {
                GiveArmor(caster);
            // Wave 15
            } else if (Match(g_magickaStealSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) MagickaSteal(target, caster);
                else RE::DebugNotification("No actor in crosshair to steal magicka from.");
            } else if (Match(g_massExecuteSpell)) {
                MassExecute(caster);
            } else if (Match(g_merchantSpell)) {
                SummonMerchant(caster);
            } else if (Match(g_magickaRegenSpell)) {
                ToggleMagickaRegen(caster);
            // Wave 16
            } else if (Match(g_staminaSurgeSpell)) {
                ToggleStaminaSurge(caster);
            } else if (Match(g_areaStaggerSpell)) {
                AreaStagger(caster);
            } else if (Match(g_goldAuraSpell)) {
                GoldAura(caster);
            } else if (Match(g_detectLifeSpell)) {
                DetectLife(caster);
            // Wave 17
            } else if (Match(g_disarmSpell)) {
                DisarmNearby(caster);
            } else if (Match(g_refillArrowsSpell)) {
                RefillArrows(caster);
            } else if (Match(g_massLevitateSpell)) {
                MassLevitate(caster);
            } else if (Match(g_soulTrapAuraSpell)) {
                SoulTrapAura(caster);
            // Wave 18
            } else if (Match(g_stealHealthSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) StealHealth(target, caster);
                else RE::DebugNotification("No actor in crosshair to steal health from.");
            } else if (Match(g_massUnlockSpell)) {
                MassUnlock(caster);
            } else if (Match(g_shieldBashSpell)) {
                ShieldBash(caster);
            } else if (Match(g_blizzardSpell)) {
                Blizzard(caster);
            // Wave 19
            } else if (Match(g_ragdollAllSpell)) {
                RagdollAll(caster);
            } else if (Match(g_equipBestSpell)) {
                EquipBest(caster);
            } else if (Match(g_massResurrectSpell)) {
                MassResurrect(caster);
            } else if (Match(g_oblivionGateSpell)) {
                OblivionGate(caster);
            // Wave 20
            } else if (Match(g_ghostFormSpell)) {
                ToggleGhostForm(caster);
            } else if (Match(g_worldquakeSpell)) {
                Worldquake(caster);
            } else if (Match(g_massParalyzeSpell)) {
                MassParalyze(caster);
            } else if (Match(g_spawnWolfPackSpell)) {
                SpawnWolfPack(caster);
            // Wave 21
            } else if (Match(g_anchorTargetSpell)) {
                if (crossRef) AnchorTarget(crossRef);
                else RE::DebugNotification("No target in crosshair to anchor.");
            } else if (Match(g_shrinkTargetSpell)) {
                if (crossRef) ShrinkTarget(crossRef);
                else RE::DebugNotification("No target in crosshair to shrink.");
            } else if (Match(g_nightEyeSpell)) {
                ToggleNightEye(caster);
            } else if (Match(g_massSilenceSpell)) {
                MassSilence(caster);
            // Wave 22
            } else if (Match(g_spawnHorseSpell)) {
                SpawnHorse(caster);
            } else if (Match(g_pickpocketAllSpell)) {
                PickpocketAll(caster);
            } else if (Match(g_massSlowSpell)) {
                MassSlow(caster);
            } else if (Match(g_spawnChestSpell)) {
                SpawnChest(caster);
            // Wave 23
            } else if (Match(g_berserkSpell)) {
                BerserkMode(caster);
            } else if (Match(g_spawnDwarvenSpell)) {
                SpawnDwarvenSphere(caster);
            } else if (Match(g_tornadoSpell)) {
                Tornado(caster);
            } else if (Match(g_petrifySpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target && !target->IsPlayerRef()) PetrifyTarget(crossRef);
                else RE::DebugNotification("No actor in crosshair to petrify.");
            // Wave 24
            } else if (Match(g_apocalypseSpell)) {
                Apocalypse(caster);
            } else if (Match(g_massDisarmSpell)) {
                MassDisarm(caster);
            } else if (Match(g_dragonShoutSpell)) {
                DragonShout(caster);
            } else if (Match(g_essenceAbsorbSpell)) {
                auto* target = crossRef ? crossRef->As<RE::Actor>() : nullptr;
                if (target) EssenceAbsorb(target, caster);
                else RE::DebugNotification("No actor in crosshair to absorb from.");
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
        // Wave 6
        MakeSpell(g_gravityPullSpell,   "[C++] Gravity Pull");
        MakeSpell(g_skeletonArmySpell,  "[C++] Skeleton Army");
        MakeSpell(g_launchSpell,        "[C++] Launch Target");
        MakeSpell(g_skillBoostSpell,    "[C++] Toggle Max Skills");
        // Wave 7
        MakeSpell(g_pacifyAllSpell,     "[C++] Pacify All");
        MakeSpell(g_identifySpell,      "[C++] Identify Target");
        MakeSpell(g_resurrectSpell,     "[C++] Resurrect");
        MakeSpell(g_armorBoostSpell,    "[C++] Toggle Iron Skin");
        // Wave 8
        MakeSpell(g_markRecallSpell,    "[C++] Mark / Recall");
        MakeSpell(g_playerGiantSpell,   "[C++] Cycle Player Scale");
        MakeSpell(g_massHealSpell,      "[C++] Mass Heal");
        MakeSpell(g_drainTargetSpell,   "[C++] Drain Life");
        // Wave 9
        MakeSpell(g_gatherNpcsSpell,    "[C++] Gather NPCs");
        MakeSpell(g_carrySpell,         "[C++] Unlimited Carry");
        MakeSpell(g_frostBarrageSpell,  "[C++] Frost Barrage");
        MakeSpell(g_snapGroundSpell,    "[C++] Snap to Ground");
        // Wave 10
        MakeSpell(g_miniaturizeSpell,   "[C++] Miniaturize All");
        MakeSpell(g_executeSpell,       "[C++] Execute");
        MakeSpell(g_bounceHouseSpell,   "[C++] Bounce House");
        MakeSpell(g_detectionSpell,     "[C++] Toggle Detection");
        // Wave 11
        MakeSpell(g_massFreezeSpell,      "[C++] Mass Freeze");
        MakeSpell(g_infiniteMagickaSpell, "[C++] Infinite Magicka");
        MakeSpell(g_deathZoneSpell,       "[C++] Death Zone");
        MakeSpell(g_superJumpSpell,       "[C++] Super Jump");
        // Wave 12
        MakeSpell(g_cloneNpcSpell,        "[C++] Clone NPC");
        MakeSpell(g_swapPosSpell,         "[C++] Swap Positions");
        MakeSpell(g_massFrenzySpell,      "[C++] Mass Frenzy");
        MakeSpell(g_scatterSpell,         "[C++] Scatter All");
        // Wave 13
        MakeSpell(g_waterBreathSpell,     "[C++] Water Breathing");
        MakeSpell(g_unlockSpell,          "[C++] Unlock Target");
        MakeSpell(g_countEnemiesSpell,    "[C++] Count Enemies");
        MakeSpell(g_timeStopSpell,        "[C++] Toggle Time Stop");
        // Wave 14
        MakeSpell(g_infernoSpell,         "[C++] Inferno");
        MakeSpell(g_atronachSpell,        "[C++] Summon Atronach");
        MakeSpell(g_cloneArmySpell,       "[C++] Clone Army");
        MakeSpell(g_giveArmorSpell,       "[C++] Give Armor");
        // Wave 15
        MakeSpell(g_magickaStealSpell,    "[C++] Magicka Steal");
        MakeSpell(g_massExecuteSpell,     "[C++] Mass Execute");
        MakeSpell(g_merchantSpell,        "[C++] Summon Merchant");
        MakeSpell(g_magickaRegenSpell,    "[C++] Instant Magicka Regen");
        // Wave 16
        MakeSpell(g_staminaSurgeSpell,    "[C++] Stamina Surge");
        MakeSpell(g_areaStaggerSpell,     "[C++] Area Stagger");
        MakeSpell(g_goldAuraSpell,        "[C++] Gold Aura");
        MakeSpell(g_detectLifeSpell,      "[C++] Detect Life");
        // Wave 21
        MakeSpell(g_anchorTargetSpell,    "[C++] Anchor Target");
        MakeSpell(g_shrinkTargetSpell,    "[C++] Shrink Target");
        MakeSpell(g_nightEyeSpell,        "[C++] Toggle Night Eye");
        MakeSpell(g_massSilenceSpell,     "[C++] Mass Silence");
        // Wave 22
        MakeSpell(g_spawnHorseSpell,      "[C++] Summon Horse");
        MakeSpell(g_pickpocketAllSpell,   "[C++] Pickpocket All");
        MakeSpell(g_massSlowSpell,        "[C++] Mass Slow");
        MakeSpell(g_spawnChestSpell,      "[C++] Spawn Chest");
        // Wave 23
        MakeSpell(g_berserkSpell,         "[C++] Berserk Mode");
        MakeSpell(g_spawnDwarvenSpell,    "[C++] Summon Dwarven Sphere");
        MakeSpell(g_tornadoSpell,         "[C++] Tornado");
        MakeSpell(g_petrifySpell,         "[C++] Petrify");
        // Wave 24
        MakeSpell(g_apocalypseSpell,      "[C++] Apocalypse");
        MakeSpell(g_massDisarmSpell,      "[C++] Mass Disarm");
        MakeSpell(g_dragonShoutSpell,     "[C++] Dragon Shout");
        MakeSpell(g_essenceAbsorbSpell,   "[C++] Essence Absorb");
        // Wave 17
        MakeSpell(g_disarmSpell,          "[C++] Disarm Nearby");
        MakeSpell(g_refillArrowsSpell,    "[C++] Refill Arrows");
        MakeSpell(g_massLevitateSpell,    "[C++] Mass Levitate");
        MakeSpell(g_soulTrapAuraSpell,    "[C++] Soul Trap Aura");
        // Wave 18
        MakeSpell(g_stealHealthSpell,     "[C++] Steal Health");
        MakeSpell(g_massUnlockSpell,      "[C++] Mass Unlock");
        MakeSpell(g_shieldBashSpell,      "[C++] Shield Bash");
        MakeSpell(g_blizzardSpell,        "[C++] Blizzard");
        // Wave 19
        MakeSpell(g_ragdollAllSpell,      "[C++] Ragdoll Wave");
        MakeSpell(g_equipBestSpell,       "[C++] Equip Best Gear");
        MakeSpell(g_massResurrectSpell,   "[C++] Mass Resurrect");
        MakeSpell(g_oblivionGateSpell,    "[C++] Oblivion Gate");
        // Wave 20
        MakeSpell(g_ghostFormSpell,       "[C++] Ghost Form");
        MakeSpell(g_worldquakeSpell,      "[C++] Worldquake");
        MakeSpell(g_massParalyzeSpell,    "[C++] Mass Paralyze");
        MakeSpell(g_spawnWolfPackSpell,   "[C++] Summon Wolf Pack");

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
            // Wave 6
            g_gravityPullSpell,
            g_skeletonArmySpell,
            g_launchSpell,
            g_skillBoostSpell,
            // Wave 7
            g_pacifyAllSpell,
            g_identifySpell,
            g_resurrectSpell,
            g_armorBoostSpell,
            // Wave 8
            g_markRecallSpell,
            g_playerGiantSpell,
            g_massHealSpell,
            g_drainTargetSpell,
            // Wave 9
            g_gatherNpcsSpell,
            g_carrySpell,
            g_frostBarrageSpell,
            g_snapGroundSpell,
            // Wave 10
            g_miniaturizeSpell,
            g_executeSpell,
            g_bounceHouseSpell,
            g_detectionSpell,
            // Wave 11
            g_massFreezeSpell,
            g_infiniteMagickaSpell,
            g_deathZoneSpell,
            g_superJumpSpell,
            // Wave 12
            g_cloneNpcSpell,
            g_swapPosSpell,
            g_massFrenzySpell,
            g_scatterSpell,
            // Wave 13
            g_waterBreathSpell,
            g_unlockSpell,
            g_countEnemiesSpell,
            g_timeStopSpell,
            // Wave 14
            g_infernoSpell,
            g_atronachSpell,
            g_cloneArmySpell,
            g_giveArmorSpell,
            // Wave 15
            g_magickaStealSpell,
            g_massExecuteSpell,
            g_merchantSpell,
            g_magickaRegenSpell,
            // Wave 16
            g_staminaSurgeSpell,
            g_areaStaggerSpell,
            g_goldAuraSpell,
            g_detectLifeSpell,
            // Wave 17
            g_disarmSpell,
            g_refillArrowsSpell,
            g_massLevitateSpell,
            g_soulTrapAuraSpell,
            // Wave 18
            g_stealHealthSpell,
            g_massUnlockSpell,
            g_shieldBashSpell,
            g_blizzardSpell,
            // Wave 19
            g_ragdollAllSpell,
            g_equipBestSpell,
            g_massResurrectSpell,
            g_oblivionGateSpell,
            // Wave 20
            g_ghostFormSpell,
            g_worldquakeSpell,
            g_massParalyzeSpell,
            g_spawnWolfPackSpell,
            // Wave 21
            g_anchorTargetSpell,
            g_shrinkTargetSpell,
            g_nightEyeSpell,
            g_massSilenceSpell,
            // Wave 22
            g_spawnHorseSpell,
            g_pickpocketAllSpell,
            g_massSlowSpell,
            g_spawnChestSpell,
            // Wave 23
            g_berserkSpell,
            g_spawnDwarvenSpell,
            g_tornadoSpell,
            g_petrifySpell,
            // Wave 24
            g_apocalypseSpell,
            g_massDisarmSpell,
            g_dragonShoutSpell,
            g_essenceAbsorbSpell,
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
