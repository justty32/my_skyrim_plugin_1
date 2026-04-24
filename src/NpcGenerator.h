#pragma once

namespace NpcGenerator
{
    // NPC Logic
    void SpawnNpc(RE::TESObjectREFR* a_anchor);
    void CustomizeNpc(RE::Actor* a_target);

    // World Logic
    void RaiseTerrain(RE::TESObjectREFR* a_anchor);
    void LowerTerrain(RE::TESObjectREFR* a_anchor);
    void PlaceTree(RE::TESObjectREFR* a_anchor);

    // Dynamic Spells
    class SpawnNpcEffect : public RE::ActiveEffect { public: void OnAdd(RE::MagicTarget*) override; };
    class CustomizeNpcEffect : public RE::ActiveEffect { public: void OnAdd(RE::MagicTarget*) override; };

    // Initialize all dynamic magic
    void InitializeMagic();
}
