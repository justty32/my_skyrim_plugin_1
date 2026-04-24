#include "NpcGenerator.h"
#include "util.h"
#include "RE/C/CrosshairPickData.h"

namespace NpcGenerator
{
    RE::SpellItem* g_spawnSpell = nullptr;
    RE::SpellItem* g_customizeSpell = nullptr;

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

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (dataHandler) {
            dataHandler->CreateReferenceAtLocation(
                newNpcBase, 
                a_anchor->GetPosition(), 
                {0, 0, 0}, 
                a_anchor->GetParentCell(), 
                a_anchor->GetWorldspace(), 
                nullptr, nullptr, RE::ObjectRefHandle(), false, true);
        }
    }

    void CustomizeNpc(RE::Actor* a_target)
    {
        if (!a_target) return;
        SKSE::log::info("CustomizeNpc: Targeting {}", a_target->GetName());

        // 做法 A: 直接調整體重 (BodySlide 的核心)
        auto* npcBase = a_target->GetActorBase();
        if (npcBase) {
            float currentWeight = npcBase->weight;
            float newWeight = currentWeight + 20.0f;
            if (newWeight > 100.0f) newWeight = 0.0f;

            npcBase->weight = newWeight;
            
            // 使用 DoReset3D 來刷新體重
            a_target->DoReset3D(true);
            SKSE::log::info("CustomizeNpc: Reset 3D with weight {}", newWeight);
        }

        // 做法 B: 開啟捏人介面
        auto* uiQueue = RE::UIMessageQueue::GetSingleton();
        if (uiQueue) {
            uiQueue->AddMessage(RE::RaceSexMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
        }
    }

    class SpellCastHandler : public RE::BSTEventSink<RE::TESSpellCastEvent>
    {
    public:
        static SpellCastHandler* GetSingleton()
        {
            static SpellCastHandler singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*) override
        {
            if (a_event && a_event->spell) {
                if (g_spawnSpell && a_event->spell == g_spawnSpell->GetFormID()) {
                    SKSE::log::info("C++: Spawn Spell Detected");
                    SpawnNpc(a_event->object.get());
                }
                else if (g_customizeSpell && a_event->spell == g_customizeSpell->GetFormID()) {
                    SKSE::log::info("C++: Customize Spell Detected");
                    
                    auto* crosshair = RE::CrosshairPickData::GetSingleton();
                    if (crosshair) {
                        auto targetRef = crosshair->targetActor[0].get();
                        if (targetRef) {
                            auto* targetActor = targetRef->As<RE::Actor>();
                            if (targetActor) {
                                CustomizeNpc(targetActor);
                            }
                        }
                    }
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void InitializeMagic()
    {
        SKSE::log::info("Initializing Dynamic Magic...");

        auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();
        if (!factory) return;

        g_spawnSpell = factory->Create()->As<RE::SpellItem>();
        g_spawnSpell->fullName = "C++: Spawn NPC";
        
        g_customizeSpell = factory->Create()->As<RE::SpellItem>();
        g_customizeSpell->fullName = "C++: Customize NPC";

        auto* source = RE::ScriptEventSourceHolder::GetSingleton();
        if (source) {
            source->AddEventSink(SpellCastHandler::GetSingleton());
        }

        SKSE::log::info("Dynamic Spells initialized and hooks registered");
    }

    void SpawnNpcEffect::OnAdd(RE::MagicTarget*) {}
    void SpawnNpcEffect::Update(float) {}
    void CustomizeNpcEffect::OnAdd(RE::MagicTarget*) {}
}
