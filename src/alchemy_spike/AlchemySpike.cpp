#include "AlchemySpike.h"

#include "RE/A/AlchemyItem.h"
#include "RE/B/BGSCreatedObjectManager.h"
#include "RE/E/Effect.h"
#include "RE/E/EffectArchetypes.h"
#include "RE/E/EffectSetting.h"
#include "RE/I/IngredientItem.h"
#include "RE/M/MagicFormulas.h"

#include <unordered_map>
#include <vector>

// =============================================================================
// AlchemySpike — see AlchemySpike.h header banner and
// research/ALCHEMY_SPIKE_FINDINGS.md.
//
// ROUTE TAKEN: reconstruct the alchemy math ourselves (route b) but defer to
// REAL vanilla functions wherever CommonLibSSE-NG exposes them:
//   * shared-effect detection  -> our own compare of IngredientItem::effects
//                                  (this is exactly what UsableEffectMap holds)
//   * skill scaling            -> RE::MagicFormulas::GetWortcraftEffectStrength
//                                  (real vanilla fn, RELOCATION exposed by NG)
//   * gold value               -> MagicItem::CalculateTotalGoldValue == the
//                                  real vanilla CalculateCost RELOCATION_ID
//                                  (11213,11321) — so the *value* is engine-
//                                  computed, not our math.
//
// Route (a) — calling the vanilla menu's internal "MakePotion/CreatePotion"
// routine directly via RELOCATION_ID — was NOT taken: CommonLibSSE-NG exposes
// no such function, and no trustworthy SE+AE-paired address-library offset for
// it could be located. We deliberately do NOT fabricate one.
// =============================================================================

namespace AlchemySpike
{
    // ---- Tunable: two hardcoded vanilla ingredients ------------------------
    // Blue Mountain Flower (0x00077E1C) + Wheat (0x00034D2C) share the
    // "Restore Health" effect in vanilla, so the brew should yield exactly one
    // shared effect (Restore Health). Both are Skyrim.esm base forms; FormIDs
    // are stable. (EditorIDs: "FoodVendorBlueMountainFlower" is unreliable; we
    // use FormIDs which the NpcGenerator pattern shows are dependable.)
    constexpr RE::FormID kIngredientA = 0x00077E1C;  // Blue Mountain Flower
    constexpr RE::FormID kIngredientB = 0x00034D2C;  // Wheat

    // ---- Trigger hotkey ----------------------------------------------------
    // DirectInput scancode for F11. Change here if it clashes.
    constexpr std::uint32_t kHotkeyScanCode = 0x57;  // DIK_F11

    // ---- vanilla GMST defaults (fallbacks only) ----------------------------
    // The actual values are read live from GameSettingCollection; these are the
    // vanilla defaults documented on UESP, used only if a GMST lookup fails.
    constexpr float kDefault_fMagicItemPriceMult = 1.5f;
    constexpr float kDefault_fAlchemyIngredientInitMult = 4.0f;
    constexpr float kDefault_fPerkAlchemyPurityBonus = 0.0f;  // perk-driven

    static float GetGameSettingFloat(const char* a_name, float a_default)
    {
        auto* gmstCollection = RE::GameSettingCollection::GetSingleton();
        if (gmstCollection) {
            auto* setting = gmstCollection->GetSetting(a_name);
            if (setting) {
                return setting->GetFloat();
            }
        }
        return a_default;
    }

    // -----------------------------------------------------------------------
    // Brew: build an AlchemyItem from the shared effects of N ingredients.
    //
    // Vanilla rule (UESP "Skyrim:Alchemy"):
    //   * An effect appears on the potion iff it is present on >= 2 of the
    //     selected ingredients (shared). With exactly 2 ingredients, that means
    //     the intersection of their effect sets.
    //   * For each shared effect, the resulting magnitude/duration is driven by
    //     the strongest contributing ingredient entry, then scaled by the
    //     alchemy skill multiplier from GetWortcraftEffectStrength + perks.
    //   * Whether an effect scales by magnitude or by duration depends on the
    //     EffectSetting flags (kNoMagnitude / kNoDuration).
    //
    // We intentionally keep this close to vanilla but flag every place a number
    // could diverge so the in-game eyeball test (see findings doc) can catch it.
    // -----------------------------------------------------------------------
    static RE::AlchemyItem* BrewPotion(RE::IngredientItem* a_ingA, RE::IngredientItem* a_ingB)
    {
        if (!a_ingA || !a_ingB) {
            SKSE::log::error("[alchemy-spike] BrewPotion: null ingredient(s)");
            return nullptr;
        }

        // --- 1. read alchemy skill + skill multiplier (real vanilla fn) ------
        auto* player = RE::PlayerCharacter::GetSingleton();
        float alchemySkill = 15.0f;  // floor; overwritten below if player valid
        if (player) {
            // 'this' is an ActorValueOwner subobject of Actor.
            alchemySkill = player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kAlchemy);
        }
        const float skillStrength = RE::MagicFormulas::GetWortcraftEffectStrength(alchemySkill);
        SKSE::log::info("[alchemy-spike] alchemy skill={:.1f}  GetWortcraftEffectStrength={:.4f}",
                        alchemySkill, skillStrength);

        // --- 2. find shared effects (intersection by baseEffect) -------------
        // Map baseEffect -> the strongest contributing Effect* across both ings.
        struct Contribution
        {
            RE::EffectSetting* base{ nullptr };
            float              magnitude{ 0.0f };
            std::uint32_t      duration{ 0 };
            std::uint32_t      area{ 0 };
        };

        std::unordered_map<RE::EffectSetting*, RE::Effect*> aEffects;
        for (auto* eff : a_ingA->effects) {
            if (eff && eff->baseEffect) {
                aEffects[eff->baseEffect] = eff;
            }
        }

        std::vector<Contribution> shared;
        for (auto* eff : a_ingB->effects) {
            if (!eff || !eff->baseEffect) {
                continue;
            }
            auto it = aEffects.find(eff->baseEffect);
            if (it == aEffects.end()) {
                continue;  // not shared
            }
            RE::Effect* fromA = it->second;
            RE::Effect* fromB = eff;

            // Vanilla takes the *stronger* contributing entry per shared effect.
            // For Restore-Health-style effects that means larger base magnitude.
            Contribution c;
            c.base = eff->baseEffect;
            c.magnitude = std::max(fromA->effectItem.magnitude, fromB->effectItem.magnitude);
            c.duration = std::max(fromA->effectItem.duration, fromB->effectItem.duration);
            c.area = std::max(fromA->effectItem.area, fromB->effectItem.area);
            shared.push_back(c);
        }

        if (shared.empty()) {
            SKSE::log::warn("[alchemy-spike] No shared effects between '{}' and '{}' — "
                            "vanilla would refuse to brew. Aborting.",
                            a_ingA->GetName(), a_ingB->GetName());
            return nullptr;
        }
        SKSE::log::info("[alchemy-spike] {} shared effect(s) found", shared.size());

        // --- 3. dynamically create the AlchemyItem (IFormFactory pattern) ----
        auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::AlchemyItem>();
        if (!factory) {
            SKSE::log::error("[alchemy-spike] No AlchemyItem form factory");
            return nullptr;
        }
        auto* potion = factory->Create()->As<RE::AlchemyItem>();
        if (!potion) {
            SKSE::log::error("[alchemy-spike] factory->Create() returned null");
            return nullptr;
        }

        // Auto-calc value: leave kCostOverride CLEAR so MagicItem::IsAutoCalc()
        // returns true and CalculateTotalGoldValue() recomputes from effects.
        potion->data.costOverride = 0;
        potion->data.flags = RE::AlchemyItem::AlchemyFlag::kNone;

        // --- 4. populate effects, applying the skill/perk scaling ------------
        bool anyDetrimental = false;
        for (const auto& c : shared) {
            auto* effect = new RE::Effect();  // TES_HEAP_REDEFINE_NEW in Effect
            effect->baseEffect = c.base;

            const auto& efData = c.base->data;
            const bool noMag = efData.flags.any(RE::EffectSetting::EffectSettingData::Flag::kNoMagnitude);
            const bool noDur = efData.flags.any(RE::EffectSetting::EffectSettingData::Flag::kNoDuration);

            // GetWortcraftEffectStrength returns the *multiplier* the alchemy
            // skill contributes. Vanilla applies it to magnitude OR duration
            // (whichever the effect uses). skillStrength ~= 1.0 at skill 0 by
            // default GMSTs and grows with skill.
            float mag = c.magnitude;
            std::uint32_t dur = c.duration;
            if (!noMag) {
                mag = c.magnitude * skillStrength;
            }
            if (noMag && !noDur) {
                // duration-based effect (e.g. Restore Stamina over time) scales
                // duration by the same multiplier
                dur = static_cast<std::uint32_t>(static_cast<float>(c.duration) * skillStrength + 0.5f);
            }

            effect->effectItem.magnitude = mag;
            effect->effectItem.duration = dur;
            effect->effectItem.area = c.area;

            potion->effects.push_back(effect);

            if (c.base->IsDetrimental()) {
                anyDetrimental = true;
            }

            SKSE::log::info("[alchemy-spike]   + effect '{}' archetype={} mag={:.2f} dur={} area={} "
                            "(noMag={} noDur={} detrimental={})",
                            c.base->GetName(), RE::EffectArchetypeToString(c.base->GetArchetype()),
                            mag, dur, c.area, noMag, noDur, c.base->IsDetrimental());
        }

        // Poison vs potion: vanilla marks it a poison when the *costliest* /
        // dominant effect is detrimental. For the spike, the simple "any
        // detrimental" heuristic is enough to eyeball; refine later.
        if (anyDetrimental) {
            potion->data.flags.set(RE::AlchemyItem::AlchemyFlag::kPoison);
        }

        // Name. Vanilla builds "Potion of <effect>" / "Poison of <effect>";
        // we set a clearly-tagged name so it's unmistakable in the inventory.
        std::string label = std::string("SPIKE ") +
                            (anyDetrimental ? "Poison of " : "Potion of ") +
                            shared.front().base->GetName();
        potion->fullName = label.c_str();

        // --- 5. ask the ENGINE for the gold value (real vanilla CalculateCost)
        const float goldValue = potion->CalculateTotalGoldValue(player);
        SKSE::log::info("[alchemy-spike] engine CalculateTotalGoldValue = {:.2f}", goldValue);

        return potion;
    }

    // -----------------------------------------------------------------------
    // Register the created potion with BGSCreatedObjectManager so it is tracked
    // and (hopefully) serialized into the save like a player-brewed potion.
    //
    // RISK / HYPOTHESIS: CommonLibSSE-NG exposes BGSCreatedObjectManager but
    // only AddArmorEnchantment / AddWeaponEnchantment — there is NO AddPotion.
    // The "potions" BSTHashMap<FormID, CreatedMagicItemData> is public, so we
    // insert directly (key = potion FormID, refCount = 1). This MIRRORS the
    // data layout the vanilla menu produces, but we cannot verify from headers
    // alone that direct insertion is sufficient for the save codec to persist
    // the dynamic form + its hashmap entry. See findings doc "persistence".
    // -----------------------------------------------------------------------
    static void RegisterCreatedPotion(RE::AlchemyItem* a_potion)
    {
        if (!a_potion) {
            return;
        }
        auto* mgr = RE::BGSCreatedObjectManager::GetSingleton();
        if (!mgr) {
            SKSE::log::error("[alchemy-spike] BGSCreatedObjectManager singleton null — "
                             "potion will NOT be registered (persistence risk)");
            return;
        }

        const auto key = a_potion->GetFormID();

        RE::BSSpinLockGuard lock(mgr->lock);
        if (mgr->potions.find(key) != mgr->potions.end()) {
            SKSE::log::info("[alchemy-spike] potion {:X} already registered", key);
            return;
        }

        RE::BGSCreatedObjectManager::CreatedMagicItemData data{};
        data.magicItem = a_potion;
        data.refCount = 1;
        data.pad0C = 0;

        mgr->potions.insert({ key, data });
        SKSE::log::info("[alchemy-spike] registered potion {:X} in BGSCreatedObjectManager::potions "
                        "(map size now {})", key, mgr->potions.size());
    }

    static void AddToPlayer(RE::AlchemyItem* a_potion)
    {
        if (!a_potion) {
            return;
        }
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            SKSE::log::error("[alchemy-spike] no player to receive potion");
            return;
        }
        player->AddObjectToContainer(a_potion, nullptr, 1, nullptr);
        SKSE::log::info("[alchemy-spike] added potion {:X} '{}' to player inventory",
                        a_potion->GetFormID(), a_potion->fullName.c_str());
    }

    // Final correctness log: dump every field of the finished potion so the
    // numbers can be compared 1:1 against the vanilla AlchemyMenu preview.
    static void LogResult(RE::AlchemyItem* a_potion)
    {
        if (!a_potion) {
            SKSE::log::warn("[alchemy-spike] LogResult: null potion (brew failed)");
            return;
        }
        SKSE::log::info("==================== ALCHEMY SPIKE RESULT ====================");
        SKSE::log::info("Name      : {}", a_potion->fullName.c_str());
        SKSE::log::info("FormID    : {:08X}", a_potion->GetFormID());
        SKSE::log::info("IsPoison  : {}", a_potion->IsPoison());
        SKSE::log::info("IsFood    : {}", a_potion->IsFood());
        SKSE::log::info("GoldValue : {:.2f}", a_potion->CalculateTotalGoldValue(RE::PlayerCharacter::GetSingleton()));
        SKSE::log::info("Effects   : {}", a_potion->effects.size());
        std::size_t i = 0;
        for (auto* eff : a_potion->effects) {
            if (!eff || !eff->baseEffect) {
                continue;
            }
            SKSE::log::info("  [{}] base='{}' (MGEF {:08X}) archetype={} mag={:.2f} dur={} area={}",
                            i++,
                            eff->baseEffect->GetName(),
                            eff->baseEffect->GetFormID(),
                            RE::EffectArchetypeToString(eff->baseEffect->GetArchetype()),
                            eff->effectItem.magnitude,
                            eff->effectItem.duration,
                            eff->effectItem.area);
        }
        SKSE::log::info("==============================================================");
    }

    // The whole brew loop, run on the main thread (game-state mutation).
    static void RunBrewSpike()
    {
        SKSE::log::info("[alchemy-spike] ---- brew triggered ----");

        auto* ingA = RE::TESForm::LookupByID<RE::IngredientItem>(kIngredientA);
        auto* ingB = RE::TESForm::LookupByID<RE::IngredientItem>(kIngredientB);
        if (!ingA || !ingB) {
            SKSE::log::error("[alchemy-spike] could not resolve ingredients {:08X} / {:08X} "
                             "(ingA={} ingB={})",
                             kIngredientA, kIngredientB,
                             static_cast<const void*>(ingA), static_cast<const void*>(ingB));
            return;
        }
        SKSE::log::info("[alchemy-spike] ingredients: '{}' + '{}'", ingA->GetName(), ingB->GetName());

        RE::AlchemyItem* potion = BrewPotion(ingA, ingB);
        if (!potion) {
            SKSE::log::warn("[alchemy-spike] brew produced no potion");
            return;
        }

        RegisterCreatedPotion(potion);
        AddToPlayer(potion);
        LogResult(potion);
    }

    // ---- input sink: fire the brew on the hotkey ---------------------------
    class HotkeySink : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        static HotkeySink* GetSingleton()
        {
            static HotkeySink singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
                                              RE::BSTEventSource<RE::InputEvent*>*) override
        {
            if (!a_event) {
                return RE::BSEventNotifyControl::kContinue;
            }
            for (auto* e = *a_event; e; e = e->next) {
                auto* button = e->AsButtonEvent();
                if (!button) {
                    continue;
                }
                if (button->GetDevice() != RE::INPUT_DEVICE::kKeyboard) {
                    continue;
                }
                if (button->GetIDCode() != kHotkeyScanCode) {
                    continue;
                }
                if (!button->IsDown()) {  // fire once on key-down only
                    continue;
                }
                // Defer game-state mutation onto the main task thread.
                if (auto* task = SKSE::GetTaskInterface()) {
                    task->AddTask([]() { RunBrewSpike(); });
                } else {
                    RunBrewSpike();
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        HotkeySink() = default;
    };

    void Init()
    {
        auto* idm = RE::BSInputDeviceManager::GetSingleton();
        if (!idm) {
            SKSE::log::error("[alchemy-spike] BSInputDeviceManager null — hotkey NOT registered");
            return;
        }
        idm->AddEventSink(HotkeySink::GetSingleton());
        SKSE::log::info("[alchemy-spike] initialized. Press F11 (scancode {:#x}) in-game to brew "
                        "a potion from ingredients {:08X} + {:08X}.",
                        kHotkeyScanCode, kIngredientA, kIngredientB);
    }
}
