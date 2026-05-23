#include "skyrim/procgen/ProcgenSpells.h"

#include "skyrim/procgen/Procgen.h"
// >>> gen-npc: demo "C++: Conjure NPC" spell -> ProcgenNpc::Generate.
#include "skyrim/procgen/ProcgenNpc.h"
// <<< gen-npc

namespace skyrim::procgen {

namespace {

// Dynamic spell pointers (NpcGenerator pattern: dynamic forms, not persisted).
RE::SpellItem* g_genRoomSpell = nullptr;
RE::SpellItem* g_conjureKeepSpell = nullptr;
RE::SpellItem* g_rearrangeSpell = nullptr;
// >>> gen-npc
RE::SpellItem* g_conjureNpcSpell = nullptr;
// <<< gen-npc

// Recipe files (live under config/procgen/, copied next to the DLL post-build).
constexpr const char* kCottageRecipe = "recipe_cottage.json";
constexpr const char* kKeepRecipe = "recipe_keep.json";

// Persist keys for the spell-generated rooms (kept distinct from the JSON-quest
// keys so the spells and the demo quest don't clobber each other).
constexpr const char* kSpellRoomKey = "spell_generated_room";
constexpr const char* kSpellKeepKey = "spell_generated_keep";

void OnGenerateRoom(RE::TESObjectREFR* anchor) {
    Recipe recipe;
    if (!LoadRecipeFile(kCottageRecipe, recipe)) {
        SKSE::log::error("ProcgenSpells: GenerateRoom — recipe load failed");
        return;
    }
    GenerateInterior(recipe, anchor, kSpellRoomKey);
}

void OnConjureKeep(RE::TESObjectREFR* anchor) {
    Recipe recipe;
    if (!LoadRecipeFile(kKeepRecipe, recipe)) {
        SKSE::log::error("ProcgenSpells: ConjureKeep — recipe load failed");
        return;
    }
    GenerateStructure(recipe, anchor, kSpellKeepKey);
}

void OnRearrange(RE::TESObjectREFR*) {
    // Rearrange the most-recently generated room (the spell-room key first, else
    // whatever was last generated). The "變更屋子擺設" request.
    RearrangeFurnishings(kSpellRoomKey);
}

// >>> gen-npc: demo recipe — mint a TESNPC_ from a vanilla template, rename, and
// place in front of the caster. Tracked + co-saved by ProcgenNpc so it rebuilds
// on reload (research/PROCGEN_NPC_FORMS.md §5/§11). The "template" is omitted so
// ProcgenNpc's vanilla placeholder (kDefaultTemplate, a generic Bandit) is used;
// set a "template" string here to mint from a different vanilla base.
void OnConjureNpc(RE::TESObjectREFR* anchor) {
    const nlohmann::json recipe = {
        {"persist_key", "spell_conjured_npc"},
        {"name", "C++ Conjured NPC"},
        {"essential", true},
        {"scale", 1.0},
    };
    const std::string key = npc::Generate(recipe, anchor);
    if (key.empty()) {
        SKSE::log::error("ProcgenSpells: ConjureNpc — generation failed");
    } else {
        SKSE::log::info("ProcgenSpells: ConjureNpc -> key='{}' ({} tracked)", key,
                        npc::TrackedCount());
    }
}
// <<< gen-npc

// Cast handler — same shape as NpcGenerator::SpellCastHandler. We register our
// own sink (rather than editing NpcGenerator's) so the merge stays isolated; the
// "C++: " prefix convention keeps both handlers cooperative.
class ProcgenCastHandler : public RE::BSTEventSink<RE::TESSpellCastEvent> {
public:
    static ProcgenCastHandler* GetSingleton() {
        static ProcgenCastHandler singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event,
                                          RE::BSTEventSource<RE::TESSpellCastEvent>*) override {
        if (!a_event) return RE::BSEventNotifyControl::kContinue;

        auto* spellForm = RE::TESForm::LookupByID(a_event->spell);
        auto* castSpell = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;
        auto* anchor = a_event->object.get();
        if (!castSpell || !anchor) return RE::BSEventNotifyControl::kContinue;

        const std::string name = castSpell->fullName.c_str();
        if (name == "C++: Generate Room") {
            SKSE::log::info("ProcgenSpells: Generate Room cast by {:X}", anchor->GetFormID());
            OnGenerateRoom(anchor);
        } else if (name == "C++: Conjure Keep") {
            SKSE::log::info("ProcgenSpells: Conjure Keep cast by {:X}", anchor->GetFormID());
            OnConjureKeep(anchor);
        } else if (name == "C++: Rearrange Furnishings") {
            SKSE::log::info("ProcgenSpells: Rearrange Furnishings cast by {:X}",
                            anchor->GetFormID());
            OnRearrange(anchor);
        // >>> gen-npc
        } else if (name == "C++: Conjure NPC") {
            SKSE::log::info("ProcgenSpells: Conjure NPC cast by {:X}", anchor->GetFormID());
            OnConjureNpc(anchor);
        // <<< gen-npc
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

}  // namespace

void InitializeSpells() {
    SKSE::log::info("ProcgenSpells: initializing dynamic procgen spells...");

    auto* spellFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::SpellItem>();
    auto* mgefFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::EffectSetting>();
    if (!spellFactory || !mgefFactory) {
        SKSE::log::error("ProcgenSpells: missing spell/effect factory");
        return;
    }

    // Dynamic base effect (no hardcoded MGEF FormID — NpcGenerator pattern).
    auto* baseEffect = mgefFactory->Create()->As<RE::EffectSetting>();
    if (!baseEffect) {
        SKSE::log::error("ProcgenSpells: failed to create base effect");
        return;
    }
    baseEffect->fullName = "Procgen Magic Effect";
    baseEffect->data.archetype = RE::EffectArchetypes::ArchetypeID::kScript;
    baseEffect->data.flags.set(RE::EffectSetting::EffectSettingData::Flag::kHideInUI);
    baseEffect->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
    baseEffect->data.delivery = RE::MagicSystem::Delivery::kSelf;

    auto CreateSpell = [&](RE::SpellItem*& a_ptr, const char* a_name) {
        a_ptr = spellFactory->Create()->As<RE::SpellItem>();
        if (!a_ptr) {
            SKSE::log::error("ProcgenSpells: failed to create spell '{}'", a_name);
            return;
        }
        a_ptr->fullName = a_name;
        a_ptr->data.spellType = RE::MagicSystem::SpellType::kLesserPower;
        a_ptr->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
        a_ptr->data.delivery = RE::MagicSystem::Delivery::kSelf;

        auto* effect = new RE::Effect();
        effect->baseEffect = baseEffect;
        effect->effectItem.magnitude = 0.0f;
        effect->effectItem.duration = 0;
        a_ptr->effects.push_back(effect);
        SKSE::log::info("ProcgenSpells: created spell '{}' ({:X})", a_name, a_ptr->GetFormID());
    };

    CreateSpell(g_genRoomSpell, "C++: Generate Room");
    CreateSpell(g_conjureKeepSpell, "C++: Conjure Keep");
    CreateSpell(g_rearrangeSpell, "C++: Rearrange Furnishings");
    // >>> gen-npc
    CreateSpell(g_conjureNpcSpell, "C++: Conjure NPC");
    // <<< gen-npc

    if (auto* source = RE::ScriptEventSourceHolder::GetSingleton()) {
        source->AddEventSink(ProcgenCastHandler::GetSingleton());
    }
    SKSE::log::info("ProcgenSpells: dynamic spells initialized");
}

void GiveSpellsToPlayer() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;
    auto AddSpell = [&](RE::SpellItem* a_spell) {
        if (a_spell) {
            player->AddSpell(a_spell);
            SKSE::log::info("ProcgenSpells: added '{}' to player", a_spell->fullName);
        }
    };
    AddSpell(g_genRoomSpell);
    AddSpell(g_conjureKeepSpell);
    AddSpell(g_rearrangeSpell);
    // >>> gen-npc
    AddSpell(g_conjureNpcSpell);
    // <<< gen-npc
}

}  // namespace skyrim::procgen
