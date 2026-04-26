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

    // --- Lifecycle ---
    void InitializeMagic();
    void GiveSpellsToPlayer();
}
