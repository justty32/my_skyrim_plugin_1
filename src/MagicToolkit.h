#pragma once

namespace MagicToolkit
{
    // --- NPC ---
    void SpawnCitizen(RE::Actor* a_caster);
    void TameNPC(RE::Actor* a_target);
    void CycleNPCScale(RE::Actor* a_target);

    // --- World ---
    void SpawnTree(RE::Actor* a_caster);
    void SpawnRock(RE::Actor* a_caster);
    void RaiseObject(RE::TESObjectREFR* a_target);
    void LowerObject(RE::TESObjectREFR* a_target);

    // --- Combat / Magic ---
    void MeteorRain(RE::Actor* a_caster);
    void ExplosionBlast(RE::TESObjectREFR* a_target, RE::Actor* a_caster);

    // --- Player Utilities ---
    void RestorePlayer(RE::Actor* a_player);
    void AddGold(RE::Actor* a_player);
    void GiveWeapon(RE::Actor* a_player);
    void InspectInventory(RE::Actor* a_player);

    // --- Wave 2 ---
    void ToggleTimeFreeze();
    void SpawnGuard(RE::Actor* a_caster);
    void TeleportToCrosshair(RE::Actor* a_player);
    void HealTarget(RE::Actor* a_target);

    // --- Wave 3 ---
    void ForcePush(RE::Actor* a_caster);
    void ToggleGodMode();
    void RandomTeleport(RE::Actor* a_player);
    void CloneObject(RE::TESObjectREFR* a_target);

    // --- Wave 4 ---
    void LevelUpPlayer(RE::PlayerCharacter* a_player);
    void DeleteTarget(RE::TESObjectREFR* a_target);
    void ParalyzeTarget(RE::Actor* a_target);
    void SpawnDragon(RE::Actor* a_caster);

    // --- Wave 5 ---
    void ToggleWeather();
    void FrenzyTarget(RE::Actor* a_target);
    void LootTarget(RE::TESObjectREFR* a_target, RE::Actor* a_player);
    void ToggleSpeedBoost(RE::Actor* a_player);

    // --- Wave 6 ---
    void GravityPull(RE::Actor* a_caster);
    void SpawnSkeletonArmy(RE::Actor* a_caster);
    void LaunchTarget(RE::Actor* a_target);
    void ToggleSkillBoost(RE::Actor* a_player);

    // --- Wave 7 ---
    void PacifyAll();
    void IdentifyTarget(RE::TESObjectREFR* a_target);
    void ResurrectTarget(RE::Actor* a_target);
    void ToggleArmorBoost(RE::Actor* a_player);

    // --- Wave 8 ---
    void MarkOrRecall(RE::Actor* a_player);
    void CyclePlayerScale(RE::Actor* a_player);
    void MassHealNearby();
    void DrainTarget(RE::Actor* a_target);

    // --- Wave 9 ---
    void GatherNPCs(RE::Actor* a_caster);
    void ToggleUnlimitedCarry(RE::Actor* a_player);
    void FrostBarrage(RE::Actor* a_caster);
    void SnapToGround(RE::TESObjectREFR* a_target, RE::Actor* a_caster);

    // --- Wave 10 ---
    void ToggleMiniaturizeAll(RE::Actor* a_caster);
    void ExecuteTarget(RE::Actor* a_target);
    void BounceHouse(RE::Actor* a_caster);
    void ToggleDetection();

    // --- Wave 11 ---
    void MassFreeze(RE::Actor* a_caster);
    void ToggleInfiniteMagicka(RE::Actor* a_player);
    void DeathZone(RE::Actor* a_caster);
    void ToggleSuperJump(RE::Actor* a_player);

    // --- Wave 12 ---
    void CloneNPC(RE::TESObjectREFR* a_target, RE::Actor* a_caster);
    void SwapPositions(RE::TESObjectREFR* a_target, RE::Actor* a_player);
    void MassFrenzy(RE::Actor* a_caster);
    void ScatterAll(RE::Actor* a_caster);

    // --- Wave 13 ---
    void ToggleWaterBreathing(RE::Actor* a_player);
    void UnlockTarget(RE::TESObjectREFR* a_target);
    void CountEnemies(RE::Actor* a_caster);
    void ToggleTimeStop();

    // --- Wave 14 ---
    void Inferno(RE::Actor* a_caster);
    void SummonAtronach(RE::Actor* a_caster);
    void CloneArmy(RE::Actor* a_caster);
    void GiveArmor(RE::Actor* a_player);

    // --- Wave 15 ---
    void MagickaSteal(RE::Actor* a_target, RE::Actor* a_player);
    void MassExecute(RE::Actor* a_caster);
    void SummonMerchant(RE::Actor* a_caster);
    void ToggleMagickaRegen(RE::Actor* a_player);

    // --- Wave 16 ---
    void ToggleStaminaSurge(RE::Actor* a_player);
    void AreaStagger(RE::Actor* a_caster);
    void GoldAura(RE::Actor* a_caster);
    void DetectLife(RE::Actor* a_caster);

    // --- Wave 17 ---
    void DisarmNearby(RE::Actor* a_caster);
    void RefillArrows(RE::Actor* a_player);
    void MassLevitate(RE::Actor* a_caster);
    void SoulTrapAura(RE::Actor* a_caster);

    // --- Wave 18 ---
    void StealHealth(RE::Actor* a_target, RE::Actor* a_player);
    void MassUnlock(RE::Actor* a_caster);
    void ShieldBash(RE::Actor* a_caster);
    void Blizzard(RE::Actor* a_caster);

    // --- Wave 19 ---
    void RagdollAll(RE::Actor* a_caster);
    void EquipBest(RE::Actor* a_player);
    void MassResurrect(RE::Actor* a_caster);
    void OblivionGate(RE::Actor* a_caster);

    // --- Wave 20 ---
    void ToggleGhostForm(RE::Actor* a_player);
    void Worldquake(RE::Actor* a_caster);
    void MassParalyze(RE::Actor* a_caster);
    void SpawnWolfPack(RE::Actor* a_caster);

    // --- Wave 21 ---
    void AnchorTarget(RE::TESObjectREFR* a_target);
    void ShrinkTarget(RE::TESObjectREFR* a_target);
    void ToggleNightEye(RE::Actor* a_player);
    void MassSilence(RE::Actor* a_caster);

    // --- Wave 22 ---
    void SpawnHorse(RE::Actor* a_caster);
    void PickpocketAll(RE::Actor* a_player);
    void MassSlow(RE::Actor* a_caster);
    void SpawnChest(RE::Actor* a_caster);

    // --- Wave 23 ---
    void BerserkMode(RE::Actor* a_player);
    void SpawnDwarvenSphere(RE::Actor* a_caster);
    void Tornado(RE::Actor* a_caster);
    void PetrifyTarget(RE::TESObjectREFR* a_target);

    // --- Wave 24 ---
    void Apocalypse(RE::Actor* a_caster);
    void MassDisarm(RE::Actor* a_caster);
    void DragonShout(RE::Actor* a_caster);
    void EssenceAbsorb(RE::Actor* a_target, RE::Actor* a_player);

    // --- Wave 25 ---
    void SummonTroll(RE::Actor* a_caster);
    void MassFlee(RE::Actor* a_caster);
    void TelekinesisPull(RE::TESObjectREFR* a_target, RE::Actor* a_player);
    void ToggleXRay(RE::Actor* a_player);

    // --- Wave 26 ---
    void SpawnBear(RE::Actor* a_caster);
    void MassCharm(RE::Actor* a_caster);
    void ToggleStoneFlesh(RE::Actor* a_player);
    void ThunderBolt(RE::TESObjectREFR* a_target, RE::Actor* a_caster);

    // --- Wave 27 ---
    void ToggleAutoHeal(RE::Actor* a_player);
    void SummonGiant(RE::Actor* a_caster);
    void MassStrength(RE::Actor* a_caster);
    void CrumbleNearby(RE::Actor* a_caster);

    // --- Wave 28 ---
    void SummonHunter(RE::Actor* a_caster);
    void MassDisplace(RE::Actor* a_caster);
    void FortifyAll(RE::Actor* a_player);
    void DivineIntervention(RE::Actor* a_player);

    // --- Lifecycle ---
    void InitializeMagic();
    void GiveSpellsToPlayer();
}
