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

    // Base Object Pointers (Pre-cached)
    RE::TESBoundObject* g_treeBase = nullptr;
    RE::TESBoundObject* g_rockBase = nullptr;
    RE::TESNPC* g_npcTemplate = nullptr;

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
        
        // Calculate position in front of the caster (200 units)
        // a_anchor->data.angle is in radians
        float angleZ = a_anchor->data.angle.z; 
        RE::NiPoint3 forwardVector;
        forwardVector.x = std::sin(angleZ);
        forwardVector.y = std::cos(angleZ);
        forwardVector.z = 0.0f;

        RE::NiPoint3 spawnPos = a_pos;
        spawnPos.x += forwardVector.x * 200.0f;
        spawnPos.y += forwardVector.y * 200.0f;
        spawnPos.z += 20.0f; // Slightly above ground

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (dataHandler) {
            auto handle = dataHandler->CreateReferenceAtLocation(
                a_base, spawnPos, a_anchor->data.angle, 
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
        if (!templateNPC || !factory) {
            SKSE::log::error("SpawnNpc: Failed to find template NPC or factory");
            return;
        }

        auto* newNpcBase = factory->Create()->As<RE::TESNPC>();
        if (!newNpcBase) {
            SKSE::log::error("SpawnNpc: Failed to create new NPC base");
            return;
        }
        newNpcBase->Copy(templateNPC);
        newNpcBase->fullName = "Generated Citizen";

        auto spawned = a_anchor->PlaceObjectAtMe(newNpcBase, false);
        if (spawned) {
             // Move it in front of the player
            float angleZ = a_anchor->data.angle.z; 
            RE::NiPoint3 forwardVector;
            forwardVector.x = std::sin(angleZ);
            forwardVector.y = std::cos(angleZ);
            forwardVector.z = 0.0f;

            RE::NiPoint3 newPos = a_anchor->GetPosition();
            newPos.x += forwardVector.x * 150.0f;
            newPos.y += forwardVector.y * 150.0f;
            newPos.z += 10.0f;

            spawned->SetPosition(newPos);
            spawned->SetAngle(a_anchor->data.angle);

            SKSE::log::info("SpawnNpc: NPC spawned and moved: {:X}", spawned->GetFormID());
        } else {
            SKSE::log::error("SpawnNpc: PlaceObjectAtMe failed");
        }
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
            SKSE::log::info("CustomizeNpc: RaceSexMenu requested");
        }
    }

    void RaiseTerrain(RE::TESObjectREFR* a_anchor)
    {
        auto* target = GetCrosshairTarget();
        if (target) {
            auto pos = target->GetPosition();
            pos.z += 50.0f;
            target->SetPosition(pos);
            SKSE::log::info("Raise: Moved object {:X} up to {}", target->GetFormID(), pos.z);
        } else {
            auto* rockBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("RockPileM01");
            if (rockBase && a_anchor) {
                auto spawned = a_anchor->PlaceObjectAtMe(rockBase, false);
                if (spawned) {
                    RE::NiPoint3 newPos = a_anchor->GetPosition();
                    newPos.z += 100.0f; // Spawn in air
                    spawned->SetPosition(newPos);
                    SKSE::log::info("Raise: No target, spawned rock at {:X}", spawned->GetFormID());
                }
            } else {
                SKSE::log::error("Raise: Failed to find rock base by EditorID");
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
            SKSE::log::info("Lower: Moved object {:X} down to {}", target->GetFormID(), pos.z);
        }
    }

    void PlaceTree(RE::TESObjectREFR* a_anchor)
    {
        if (!a_anchor) return;
        
        // Use EditorID lookup - much more reliable than hardcoded FormIDs in dynamic environments
        auto* treeBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("TreeFloraJuniper01");
        if (!treeBase) {
            SKSE::log::warn("PlaceTree: Could not find 'TreeFloraJuniper01', trying fallback...");
            treeBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("RockPileM01");
        }

        if (treeBase) {
            // PlaceObjectAtMe is the most reliable spawning API (same as console placeatme)
            auto spawned = a_anchor->PlaceObjectAtMe(treeBase, false);
            if (spawned) {
                // Move it in front of the player so they can see it
                float angleZ = a_anchor->data.angle.z; 
                RE::NiPoint3 forwardVector;
                forwardVector.x = std::sin(angleZ);
                forwardVector.y = std::cos(angleZ);
                forwardVector.z = 0.0f;

                RE::NiPoint3 newPos = a_anchor->GetPosition();
                newPos.x += forwardVector.x * 200.0f;
                newPos.y += forwardVector.y * 200.0f;
                newPos.z += 10.0f;

                spawned->SetPosition(newPos);
                spawned->SetAngle(a_anchor->data.angle);
                
                SKSE::log::info("PlaceTree: Object spawned and moved to front: {:X}", spawned->GetFormID());
            } else {
                SKSE::log::error("PlaceTree: PlaceObjectAtMe returned null");
            }
        } else {
            SKSE::log::error("PlaceTree: Failed to find any base object by EditorID");
        }
    }

    // --- Event Handler ---

    class SpellCastHandler : public RE::BSTEventSink<RE::TESSpellCastEvent>
    {
    public:
        static SpellCastHandler* GetSingleton() { static SpellCastHandler singleton; return &singleton; }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*) override
        {
            if (!a_event) return RE::BSEventNotifyControl::kContinue;

            auto* spellForm = RE::TESForm::LookupByID(a_event->spell);
            auto* castSpell = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;
            auto* anchor = a_event->object.get();

            if (!castSpell || !anchor) return RE::BSEventNotifyControl::kContinue;

            std::string spellName = castSpell->fullName.c_str();
            
            if (spellName.find("C++: ") == 0) {
                SKSE::log::info("Custom Spell Detected: {} cast by {:X}", spellName, anchor->GetFormID());
                
                if (spellName == "C++: Spawn NPC") {
                    SpawnNpc(anchor);
                }
                else if (spellName == "C++: Customize NPC") {
                    auto* target = GetCrosshairTarget();
                    if (target && target->As<RE::Actor>()) {
                        CustomizeNpc(target->As<RE::Actor>());
                    } else {
                        SKSE::log::warn("CustomizeNpc: No actor target found under crosshair");
                    }
                }
                else if (spellName == "C++: Raise Terrain/Object") {
                    RaiseTerrain(anchor);
                }
                else if (spellName == "C++: Lower Terrain/Object") {
                    LowerTerrain(anchor);
                }
                else if (spellName == "C++: Place Tree") {
                    PlaceTree(anchor);
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void InitializeMagic()
    {
        SKSE::log::info("Initializing Dynamic Magic Spells...");
        
        // Use absolute ID lookup for Skyrim.esm base forms
        g_treeBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00038432);
        g_rockBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0001B983);
        g_npcTemplate = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);

        if (g_treeBase) SKSE::log::info("Pre-cached Tree: {:X}", g_treeBase->GetFormID());
        else SKSE::log::error("Failed to pre-cache Tree (0x00038432)!");

        if (g_rockBase) SKSE::log::info("Pre-cached Rock: {:X}", g_rockBase->GetFormID());
        else SKSE::log::error("Failed to pre-cache Rock (0x0001B983)!");

        if (g_npcTemplate) SKSE::log::info("Pre-cached NPC Template: {:X}", g_npcTemplate->GetFormID());
        else SKSE::log::error("Failed to pre-cache NPC Template (0x7)!");

        if (g_npcTemplate) SKSE::log::info("Pre-cached NPC Template: {:X}", g_npcTemplate->GetFormID());
        else SKSE::log::error("Failed to pre-cache NPC Template (0x7)!");

        auto* spellFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();
        auto* mgefFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::EffectSetting>();
        
        if (!spellFactory || !mgefFactory) return;

        // Dynamically create a base effect to avoid dependency on hardcoded FormIDs
        auto* baseEffect = mgefFactory->Create()->As<RE::EffectSetting>();
        if (baseEffect) {
            baseEffect->fullName = "NpcGenerator Magic Effect";
            baseEffect->data.archetype = RE::EffectArchetypes::ArchetypeID::kScript;
            baseEffect->data.flags.set(RE::EffectSetting::EffectSettingData::Flag::kHideInUI);
            baseEffect->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
            baseEffect->data.delivery = RE::MagicSystem::Delivery::kSelf;
            SKSE::log::info("Created dynamic base effect (ID: {:X})", baseEffect->GetFormID());
        } else {
            SKSE::log::error("Failed to create dynamic base effect");
            return;
        }

        auto CreateSpell = [&](RE::SpellItem*& a_ptr, const char* a_name) {
            a_ptr = spellFactory->Create()->As<RE::SpellItem>();
            if (a_ptr) {
                a_ptr->fullName = a_name;
                // Set as a Lesser Power so it's easy to use
                a_ptr->data.spellType = RE::MagicSystem::SpellType::kLesserPower;
                a_ptr->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
                a_ptr->data.delivery = RE::MagicSystem::Delivery::kSelf;

                auto* effect = new RE::Effect();
                effect->baseEffect = baseEffect;
                effect->effectItem.magnitude = 0.0f;
                effect->effectItem.duration = 0;

                a_ptr->effects.push_back(effect);
                SKSE::log::info("Created spell: {} (ID: {:X})", a_name, a_ptr->GetFormID());
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
