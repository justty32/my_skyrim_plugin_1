#include "NpcGenerator.h"
#include "util.h"
#include "RE/C/CrosshairPickData.h"

namespace NpcGenerator
{
    // Spell Pointers
    RE::SpellItem* g_spawnSpell = nullptr;
    RE::SpellItem* g_customizeSpell = nullptr;
    RE::SpellItem* g_raiseSpell = nullptr;
    RE::SpellItem* g_lowerSpell = nullptr;
    RE::SpellItem* g_treeSpell = nullptr;

    // --- Helpers ---

    RE::TESObjectREFR* GetCrosshairTarget() {
        auto* crosshair = RE::CrosshairPickData::GetSingleton();
        if (crosshair) {
            auto targetRef = crosshair->target[0].get();
            return targetRef.get();
        }
        return nullptr;
    }

    RE::TESObjectREFR* SpawnAtLocation(RE::TESBoundObject* a_base, RE::NiPoint3 a_pos, RE::TESObjectREFR* a_anchor) {
        if (!a_base || !a_anchor) return nullptr;
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (dataHandler) {
            auto handle = dataHandler->CreateReferenceAtLocation(
                a_base, a_pos, {0, 0, 0}, 
                a_anchor->GetParentCell(), a_anchor->GetWorldspace(), 
                nullptr, nullptr, RE::ObjectRefHandle(), false, true);
            
            return handle.get().get();
        }
        return nullptr;
    }

    // --- Core Logic ---

    void SpawnNpc(RE::TESObjectREFR* a_anchor)
    {
        if (!a_anchor) return;
        auto* templateNPC = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);
        auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
        if (!templateNPC || !factory) return;

        auto* newNpcBase = factory->Create()->As<RE::TESNPC>();
        if (!newNpcBase) return;
        newNpcBase->Copy(templateNPC);
        newNpcBase->fullName = "Generated Citizen";

        SpawnAtLocation(newNpcBase, a_anchor->GetPosition(), a_anchor);
    }

    void CustomizeNpc(RE::Actor* a_target)
    {
        if (!a_target) return;
        auto* npcBase = a_target->GetActorBase();
        if (npcBase) {
            float currentWeight = npcBase->weight;
            float newWeight = (currentWeight >= 100.0f) ? 0.0f : currentWeight + 20.0f;
            npcBase->weight = newWeight;
            a_target->DoReset3D(true);
            SKSE::log::info("CustomizeNpc: Target weight set to {}", newWeight);
        }
        auto* uiQueue = RE::UIMessageQueue::GetSingleton();
        if (uiQueue) {
            uiQueue->AddMessage(RE::RaceSexMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
        }
    }

    void RaiseTerrain(RE::TESObjectREFR* a_anchor)
    {
        auto* target = GetCrosshairTarget();
        if (target) {
            auto pos = target->GetPosition();
            pos.z += 50.0f;
            target->SetPosition(pos);
            SKSE::log::info("Raise: Moved object up");
        } else {
            auto* rockBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0001B983);
            if (rockBase && a_anchor) {
                SpawnAtLocation(rockBase, a_anchor->GetPosition(), a_anchor);
            }
        }
    }

    void LowerTerrain(RE::TESObjectREFR* a_anchor)
    {
        auto* target = GetCrosshairTarget();
        if (target) {
            auto pos = target->GetPosition();
            pos.z -= 50.0f;
            target->SetPosition(pos);
            SKSE::log::info("Lower: Moved object down");
        }
    }

    void PlaceTree(RE::TESObjectREFR* a_anchor)
    {
        if (!a_anchor) return;
        auto* treeBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00038432);
        if (treeBase) {
            SpawnAtLocation(treeBase, a_anchor->GetPosition(), a_anchor);
            SKSE::log::info("PlaceTree: Tree spawned");
        }
    }

    // --- Event Handler ---

    class SpellCastHandler : public RE::BSTEventSink<RE::TESSpellCastEvent>
    {
    public:
        static SpellCastHandler* GetSingleton() { static SpellCastHandler singleton; return &singleton; }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*) override
        {
            if (!a_event || !a_event->spell) return RE::BSEventNotifyControl::kContinue;

            auto spellID = a_event->spell;
            auto* anchor = a_event->object.get();

            if (g_spawnSpell && spellID == g_spawnSpell->GetFormID()) {
                SpawnNpc(anchor);
            }
            else if (g_customizeSpell && spellID == g_customizeSpell->GetFormID()) {
                auto* target = GetCrosshairTarget();
                if (target && target->As<RE::Actor>()) CustomizeNpc(target->As<RE::Actor>());
            }
            else if (g_raiseSpell && spellID == g_raiseSpell->GetFormID()) {
                RaiseTerrain(anchor);
            }
            else if (g_lowerSpell && spellID == g_lowerSpell->GetFormID()) {
                LowerTerrain(anchor);
            }
            else if (g_treeSpell && spellID == g_treeSpell->GetFormID()) {
                PlaceTree(anchor);
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void InitializeMagic()
    {
        SKSE::log::info("Initializing Dynamic Magic Spells...");
        auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();
        if (!factory) return;

        auto CreateSpell = [&](RE::SpellItem*& a_ptr, const char* a_name) {
            a_ptr = factory->Create()->As<RE::SpellItem>();
            if (a_ptr) {
                a_ptr->fullName = a_name;
                // Set as a Lesser Power so it's easy to use
                a_ptr->data.spellType = RE::MagicSystem::SpellType::kLesserPower;
            }
        };

        CreateSpell(g_spawnSpell, "C++: Spawn NPC");
        CreateSpell(g_customizeSpell, "C++: Customize NPC");
        CreateSpell(g_raiseSpell, "C++: Raise Terrain/Object");
        CreateSpell(g_lowerSpell, "C++: Lower Terrain/Object");
        CreateSpell(g_treeSpell, "C++: Place Tree");

        auto* source = RE::ScriptEventSourceHolder::GetSingleton();
        if (source) source->AddEventSink(SpellCastHandler::GetSingleton());

        SKSE::log::info("All dynamic spells initialized.");
    }

    void GiveSpellsToPlayer()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        auto AddSpell = [&](RE::SpellItem* a_spell) {
            if (a_spell) {
                player->AddSpell(a_spell);
                SKSE::log::info("Added spell '{}' to player", a_spell->fullName);
            }
        };

        AddSpell(g_spawnSpell);
        AddSpell(g_customizeSpell);
        AddSpell(g_raiseSpell);
        AddSpell(g_lowerSpell);
        AddSpell(g_treeSpell);
    }

    void SpawnNpcEffect::OnAdd(RE::MagicTarget*) {}
    void CustomizeNpcEffect::OnAdd(RE::MagicTarget*) {}
}
