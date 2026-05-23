#include "skyrim/procgen/ProcgenItem.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <SKSE/API.h>
#include <SKSE/Interfaces.h>

#include "skyrim/CoSave.h"  // central co-save dispatcher (one SetUniqueID per plugin)
#include "util.h"           // FormUtil::Parse, Util::String

namespace skyrim::procgen::item {

namespace {

// Item kinds we can mint. The recipe "type" string selects one.
enum class Kind { kWeapon, kArmor, kMisc, kUnknown };

Kind ParseKind(const std::string& s) {
    const std::string k = Util::String::ToLower(s);
    if (k == "weapon" || k == "weap") return Kind::kWeapon;
    if (k == "armor" || k == "armour" || k == "armo") return Kind::kArmor;
    if (k == "misc" || k == "miscellaneous") return Kind::kMisc;
    return Kind::kUnknown;
}

const char* KindName(Kind k) {
    switch (k) {
        case Kind::kWeapon: return "weapon";
        case Kind::kArmor: return "armor";
        case Kind::kMisc: return "misc";
        default: return "unknown";
    }
}

// Default vanilla templates per type. Copy()/component-copy of these yields an
// immediately-usable item (model/keywords/sounds inherited) without hand-filling.
// >>> formids
// All three are VERIFIED-common Skyrim.esm base FormIDs widely documented on the
// UESP item-code / console pages:
//   0x00012EB7 = Iron Sword   (TESObjectWEAP, one-handed sword)
//   0x00012E49 = Iron Armor   (TESObjectARMO, cuirass body slot)
//   0x0000000F = Gold001      (TESObjectMISC, the gold "coin" misc item)
// Override per-recipe via the recipe's "template" field.
constexpr RE::FormID kDefaultWeaponTemplate = 0x00012EB7;  // Iron Sword
constexpr RE::FormID kDefaultArmorTemplate = 0x00012E49;   // Iron Armor (cuirass)
constexpr RE::FormID kDefaultMiscTemplate = 0x0000000F;    // Gold001
// <<< formids

RE::FormID DefaultTemplate(Kind k) {
    switch (k) {
        case Kind::kWeapon: return kDefaultWeaponTemplate;
        case Kind::kArmor: return kDefaultArmorTemplate;
        case Kind::kMisc: return kDefaultMiscTemplate;
        default: return kDefaultMiscTemplate;
    }
}

// One runtime-tracked generated item. We keep the recipe JSON verbatim (rebuild
// source of truth), the resolved template's *plugin* FormID (ResolveFormID-safe;
// NEVER the 0xFF dynamic id — research §5 step 2), and the live minted base form's
// pointer for this session only (so OnSave can refresh, and rebuild can strip a
// stale count). We never persist that pointer or the dynamic id.
struct TrackedItem {
    std::string key;                  // stable string id (logical identity)
    Kind kind = Kind::kUnknown;
    RE::FormID templatePluginFormID;  // EXISTING plugin form, ResolveFormID-safe
    std::string recipeJson;           // full recipe (rebuild source of truth)
    std::int32_t count = 1;           // how many were added to the player
    RE::TESBoundObject* live = nullptr;  // session-local minted base (not persisted)
};

std::mutex g_mutex;
std::unordered_map<std::string, TrackedItem>& Registry() {
    static std::unordered_map<std::string, TrackedItem> reg;
    return reg;
}

// Recipes read by OnLoad, awaiting a main-thread rebuild in RebuildStaged()
// (OnLoad runs off the main thread, so we cannot mint there — research §5 "時機").
struct StagedRecipe {
    std::string key;
    Kind kind = Kind::kUnknown;
    RE::FormID templatePluginFormID;  // already remapped by ResolveFormID
    std::string recipeJson;
    std::int32_t count = 1;
};
std::vector<StagedRecipe> g_staged;

std::uint32_t& AutoKeyCounter() {
    static std::uint32_t n = 0;
    return n;
}

// Resolve a recipe "template" string to a TESForm + remember its plugin FormID.
// Accepts "0x..~Mod.esp" (FormUtil::Parse), bare "0x.." absolute id, or EditorID
// (mirrors SkyrimEntities::resolveForm so the vocabulary is consistent).
RE::TESForm* ResolveTemplate(const std::string& ref, Kind kind, RE::FormID& outPluginFormID) {
    RE::TESForm* form = nullptr;
    if (!ref.empty()) {
        if (ref.find('~') != std::string::npos) {
            form = FormUtil::Parse::GetFormFromConfigString(ref);
        } else if (ref.size() > 1 && ref[0] == '0' && (ref[1] == 'x' || ref[1] == 'X')) {
            try {
                const auto id = static_cast<RE::FormID>(std::stoul(ref, nullptr, 16));
                form = RE::TESForm::LookupByID(id);
            } catch (...) {
            }
        } else {
            form = RE::TESForm::LookupByEditorID(ref);
        }
    }
    if (!form) {
        form = RE::TESForm::LookupByID(DefaultTemplate(kind));  // placeholder fallback
    }
    // Only an EXISTING plugin form has a ResolveFormID-safe id (research §3.3).
    if (form && form->GetFormID() < 0xFF000000) {
        outPluginFormID = form->GetFormID();
    } else {
        outPluginFormID = DefaultTemplate(kind);
    }
    return form;
}

// ---- per-type deep field copy (see header: WEAP/MISC do NOT override Copy) ----
// Component-copy the BaseFormComponent sub-objects shared by these item types via
// their CopyComponent (vfunc 03). Both `dst` and `src` derive from each component,
// so the cast is to the SAME sub-object on each side.

void CopyModel(RE::TESModelTextureSwap* dst, RE::TESModelTextureSwap* src) {
    if (dst && src) {
        // TESModelTextureSwap : TESModel : BaseFormComponent — CopyComponent deep-
        // copies the MODL path + texture/addon arrays.
        static_cast<RE::TESModel*>(dst)->CopyComponent(static_cast<RE::TESModel*>(src));
    }
}
void CopyName(RE::TESFullName* dst, RE::TESFullName* src) {
    if (dst && src) dst->CopyComponent(src);
}
void CopyValue(RE::TESValueForm* dst, RE::TESValueForm* src) {
    if (dst && src) dst->CopyComponent(src);
}
void CopyWeight(RE::TESWeightForm* dst, RE::TESWeightForm* src) {
    if (dst && src) dst->CopyComponent(src);
}
void CopyKeywords(RE::BGSKeywordForm* dst, RE::BGSKeywordForm* src) {
    if (dst && src) dst->CopyComponent(src);
}

// Mint a WEAP by component-copy + raw weapon-struct copy (Copy is a no-op for WEAP).
RE::TESObjectWEAP* MintWeapon(RE::TESObjectWEAP* tmpl) {
    auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectWEAP>();
    if (!factory || !tmpl) return nullptr;
    auto* w = factory->Create()->As<RE::TESObjectWEAP>();
    if (!w) return nullptr;
    CopyModel(w, tmpl);
    CopyName(w, tmpl);
    CopyValue(w, tmpl);
    CopyWeight(w, tmpl);
    CopyKeywords(w, tmpl);
    // Enchantable component (formEnchanting/castingType) — share template's state.
    static_cast<RE::TESEnchantableForm*>(w)->CopyComponent(
        static_cast<RE::TESEnchantableForm*>(tmpl));
    // Type-specific raw data: DNAM (speed/reach/skill/anim type/flags), attack
    // damage, and crit. These are plain PODs on the form EXCEPT weaponData.rangedData,
    // which is an OWNED heap pointer (RangedData*) used by ranged weapons (bows/
    // crossbows). The memberwise copy below aliases that pointer, so a ranged
    // template would share one RangedData between the minted weapon and the vanilla
    // template (double-free / shared-edit). Deep-copy it after the struct copy so
    // the minted weapon owns its own RangedData; null stays null (melee templates).
    w->weaponData = tmpl->weaponData;
    if (tmpl->weaponData.rangedData) {
        w->weaponData.rangedData = new RE::TESObjectWEAP::RangedData(*tmpl->weaponData.rangedData);
    }
    w->criticalData = tmpl->criticalData;
    static_cast<RE::TESAttackDamageForm*>(w)->attackDamage =
        static_cast<RE::TESAttackDamageForm*>(tmpl)->attackDamage;
    // Sounds / impact / first-person model / soundLevel: inherit the template's
    // pointers so the minted weapon swings/equips with audio + a 1st-person model.
    w->attackSound = tmpl->attackSound;
    w->attackSound2D = tmpl->attackSound2D;
    w->attackLoopSound = tmpl->attackLoopSound;
    w->attackFailSound = tmpl->attackFailSound;
    w->idleSound = tmpl->idleSound;
    w->equipSound = tmpl->equipSound;
    w->unequipSound = tmpl->unequipSound;
    w->impactDataSet = tmpl->impactDataSet;
    w->firstPersonModelObject = tmpl->firstPersonModelObject;
    w->soundLevel = tmpl->soundLevel;
    return w;
}

// Mint an ARMO via its REAL Copy override (deep-copies biped model + addons +
// armorRating — TESObjectARMO.h:61). This is the clean, header-verified path.
RE::TESObjectARMO* MintArmor(RE::TESObjectARMO* tmpl) {
    auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectARMO>();
    if (!factory || !tmpl) return nullptr;
    auto* a = factory->Create()->As<RE::TESObjectARMO>();
    if (!a) return nullptr;
    a->Copy(tmpl);  // vfunc 2F override: full armor deep copy
    return a;
}

// Mint a MISC by component-copy (Copy is a no-op for MISC, like WEAP).
RE::TESObjectMISC* MintMisc(RE::TESObjectMISC* tmpl) {
    auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectMISC>();
    if (!factory || !tmpl) return nullptr;
    auto* m = factory->Create()->As<RE::TESObjectMISC>();
    if (!m) return nullptr;
    CopyModel(m, tmpl);
    CopyName(m, tmpl);
    CopyValue(m, tmpl);
    CopyWeight(m, tmpl);
    CopyKeywords(m, tmpl);
    return m;
}

// Apply the optional recipe overrides onto a freshly minted base. Returns the
// minted base as a TESBoundObject (the common base for AddObjectToContainer).
RE::TESBoundObject* ApplyOverridesAndCast(RE::TESForm* minted, Kind kind,
                                          const nlohmann::json& recipe) {
    if (!minted) return nullptr;

    const std::string name = recipe.value("name", std::string{});

    // value/weight live on the TESValueForm / TESWeightForm component bases. NOTE:
    // TESForm::As<T> is a FORM-TYPE switch (FormTraits.h:146) and only maps to
    // concrete form classes — it returns null for component bases like
    // TESValueForm. So we static_cast the CONCRETE pointer (obtained per-kind
    // below) to the component base instead. Applied via this local helper.
    const auto applyValueWeight = [&](RE::TESValueForm* vf, RE::TESWeightForm* wf) {
        if (vf && recipe.contains("value") && recipe["value"].is_number()) {
            vf->value = recipe["value"].get<std::int32_t>();
        }
        if (wf && recipe.contains("weight") && recipe["weight"].is_number()) {
            wf->weight = recipe["weight"].get<float>();
        }
    };

    switch (kind) {
        case Kind::kWeapon: {
            auto* w = minted->As<RE::TESObjectWEAP>();
            if (!w) return nullptr;
            applyValueWeight(static_cast<RE::TESValueForm*>(w), static_cast<RE::TESWeightForm*>(w));
            if (!name.empty()) w->SetFullName(name.c_str());
            if (recipe.contains("damage") && recipe["damage"].is_number()) {
                const auto dmg = recipe["damage"].get<int>();
                if (dmg >= 0)
                    static_cast<RE::TESAttackDamageForm*>(w)->attackDamage =
                        static_cast<std::uint16_t>(dmg);
            }
            return w;
        }
        case Kind::kArmor: {
            auto* a = minted->As<RE::TESObjectARMO>();
            if (!a) return nullptr;
            applyValueWeight(static_cast<RE::TESValueForm*>(a), static_cast<RE::TESWeightForm*>(a));
            if (!name.empty()) a->SetFullName(name.c_str());
            if (recipe.contains("armor") && recipe["armor"].is_number()) {
                const auto rating = recipe["armor"].get<int>();
                // armorRating is stored as CK-points * 100 (TESObjectARMO.h:71).
                if (rating >= 0)
                    a->armorRating = static_cast<std::uint32_t>(rating) * 100u;
            }
            return a;
        }
        case Kind::kMisc: {
            auto* m = minted->As<RE::TESObjectMISC>();
            if (!m) return nullptr;
            applyValueWeight(static_cast<RE::TESValueForm*>(m), static_cast<RE::TESWeightForm*>(m));
            if (!name.empty()) m->SetFullName(name.c_str());
            return m;
        }
        default:
            return nullptr;
    }
}

// Core mint dispatch by kind. Returns the minted base (null on failure).
RE::TESBoundObject* MintItem(RE::TESForm* tmpl, Kind kind, const nlohmann::json& recipe) {
    RE::TESForm* minted = nullptr;
    switch (kind) {
        case Kind::kWeapon:
            minted = MintWeapon(tmpl ? tmpl->As<RE::TESObjectWEAP>() : nullptr);
            break;
        case Kind::kArmor:
            minted = MintArmor(tmpl ? tmpl->As<RE::TESObjectARMO>() : nullptr);
            break;
        case Kind::kMisc:
            minted = MintMisc(tmpl ? tmpl->As<RE::TESObjectMISC>() : nullptr);
            break;
        default:
            SKSE::log::error("ProcgenItem: unknown item kind");
            return nullptr;
    }
    if (!minted) {
        SKSE::log::error("ProcgenItem: mint failed for kind '{}'", KindName(kind));
        return nullptr;
    }
    auto* obj = ApplyOverridesAndCast(minted, kind, recipe);
    if (obj) {
        SKSE::log::info("ProcgenItem: minted {} base {:X} (template {:X})", KindName(kind),
                        obj->GetFormID(), tmpl ? tmpl->GetFormID() : 0);
    }
    return obj;
}

// The vanilla FormType minted for each Kind (used by the dedup strip below).
RE::FormType FormTypeOf(Kind k) {
    switch (k) {
        case Kind::kWeapon: return RE::FormType::Weapon;
        case Kind::kArmor: return RE::FormType::Armor;
        case Kind::kMisc: return RE::FormType::Misc;
        default: return RE::FormType::None;
    }
}

// Strip prior instances of a generated item from the player so each persist_key
// ends up with EXACTLY ONE instance after a re-Generate / rebuild (header §35-38).
// We remove (best-effort):
//   * the live tracked base from THIS session, if any (RemoveItem by pointer), and
//   * any DYNAMIC (0xFF...) base of the same form type whose name matches the
//     recipe's name — these are the prior session's minted base that the vanilla
//     codec serialized into the player's inventory and restored on load. We match
//     on the dynamic-id range + form type + name so we never strip a plugin item
//     or another mod's dynamic item of a different name.
// `live` may be null/stale across a reload (the registry is cleared on revert);
// the inventory scan covers the cross-reload case, the pointer covers same-session
// re-Generate. Counts are capped to what we are about to (re)add so we never go
// negative on a partially-consumed stack.
void StripPriorInstances(RE::PlayerCharacter* player, Kind kind, const std::string& name,
                         RE::TESBoundObject* live, std::int32_t count) {
    if (!player || count <= 0) return;
    const RE::FormType ft = FormTypeOf(kind);

    // 1) Same-session: remove the previously minted live base directly.
    if (live && live->GetFormType() == ft) {
        player->RemoveItem(live, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
    }

    // 2) Cross-reload: remove the codec-restored prior dynamic base(s). Only match
    //    DYNAMIC ids of the same form type whose name equals the recipe name (when
    //    a name override exists — a blank shell with no name is left alone, as the
    //    header's persistence note documents we cannot reliably match it).
    if (name.empty()) return;
    auto inv = player->GetInventory([ft](RE::TESBoundObject& obj) {
        return obj.GetFormType() == ft && obj.GetFormID() >= 0xFF000000;
    });
    for (auto& [obj, data] : inv) {
        if (!obj || obj == live) continue;  // live already handled above
        const char* objName = obj->GetName();
        if (!objName || name != objName) continue;
        const std::int32_t have = data.first;
        if (have <= 0) continue;
        const std::int32_t toRemove = have < count ? have : count;
        player->RemoveItem(obj, toRemove, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
        SKSE::log::info("ProcgenItem: stripped {} prior '{}' (dynamic base {:X}) before rebuild",
                        toRemove, objName, obj->GetFormID());
    }
}

// ---- co-save blob (de)serialization helpers (mirrors ProcgenNpc) ----

bool WriteString(SKSE::SerializationInterface* intfc, const std::string& s) {
    const auto len = static_cast<std::uint32_t>(s.size());
    if (!intfc->WriteRecordData(len)) return false;
    if (len == 0) return true;
    return intfc->WriteRecordData(s.data(), len);
}

bool ReadString(SKSE::SerializationInterface* intfc, std::string& out) {
    std::uint32_t len = 0;
    if (intfc->ReadRecordData(len) == 0) return false;
    out.clear();
    if (len == 0) return true;
    if (len > (1u << 20)) {  // 1 MiB sanity cap — a corrupt length must not OOM
        SKSE::log::error("ProcgenItem: refusing absurd string length {}", len);
        return false;
    }
    out.resize(len);
    return intfc->ReadRecordData(out.data(), len) == len;
}

}  // namespace

// ---------------------------------------------------------------------------
// Generation core — public entry from the adapter / demo spell.
// ---------------------------------------------------------------------------

std::string Generate(const nlohmann::json& recipe) {
    if (!recipe.is_object()) {
        SKSE::log::warn("ProcgenItem: Generate called with a non-object recipe");
        return {};
    }
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::error("ProcgenItem: Generate has no player");
        return {};
    }

    const Kind kind = ParseKind(recipe.value("type", std::string{}));
    if (kind == Kind::kUnknown) {
        SKSE::log::warn("ProcgenItem: Generate missing/invalid 'type' (weapon|armor|misc)");
        return {};
    }

    RE::FormID templatePluginFormID = DefaultTemplate(kind);
    auto* tmpl = ResolveTemplate(recipe.value("template", std::string{}), kind, templatePluginFormID);

    auto* obj = MintItem(tmpl, kind, recipe);
    if (!obj) return {};

    std::int32_t count = recipe.value("count", 1);
    if (count <= 0) count = 1;

    std::string key = recipe.value("persist_key", std::string{});
    if (key.empty()) key = "gen_item_" + std::to_string(AutoKeyCounter()++);

    // If this key was already generated this session, strip the prior live base
    // first so a re-Generate of the same key leaves exactly one instance (no
    // accumulation — header §35-38).
    {
        std::scoped_lock lock(g_mutex);
        if (auto it = Registry().find(key); it != Registry().end()) {
            StripPriorInstances(player, it->second.kind, recipe.value("name", std::string{}),
                                it->second.live, it->second.count);
        }
    }

    // Add the minted base to the player's inventory (NpcGenerator / alchemy
    // convention — TESObjectREFR::AddObjectToContainer, vfunc 5A).
    player->AddObjectToContainer(obj, nullptr, count, nullptr);

    TrackedItem t;
    t.key = key;
    t.kind = kind;
    t.templatePluginFormID = templatePluginFormID;
    t.recipeJson = recipe.dump();
    t.count = count;
    t.live = obj;
    {
        std::scoped_lock lock(g_mutex);
        Registry()[key] = std::move(t);
    }
    SKSE::log::info("ProcgenItem: gave {} '{}' x{} (key='{}', template {:X}) to player",
                    KindName(kind), obj->GetName(), count, key, templatePluginFormID);
    return key;
}

// ---------------------------------------------------------------------------
// Co-save persistence — store the recipe + template plugin FormID, rebuild on load.
// ---------------------------------------------------------------------------

void OnSave(SKSE::SerializationInterface* intfc) {
    std::scoped_lock lock(g_mutex);
    auto& reg = Registry();

    if (!intfc->OpenRecord(kRecordType, kRecordVersion)) {
        SKSE::log::error("ProcgenItem: OnSave OpenRecord failed");
        return;
    }
    const auto count = static_cast<std::uint32_t>(reg.size());
    intfc->WriteRecordData(count);

    for (auto& [key, t] : reg) {
        WriteString(intfc, key);
        const auto kindByte = static_cast<std::uint32_t>(t.kind);
        intfc->WriteRecordData(kindByte);
        // Store the EXISTING template plugin FormID — ResolveFormID remaps it on
        // load. NEVER the 0xFF dynamic minted-base id (research §5 step 2 / §3.1).
        intfc->WriteRecordData(t.templatePluginFormID);
        intfc->WriteRecordData(t.count);
        WriteString(intfc, t.recipeJson);
    }
    SKSE::log::info("ProcgenItem: OnSave wrote {} generated-item recipes", count);
}

void OnLoad(SKSE::SerializationInterface* intfc, std::uint32_t version, std::uint32_t /*length*/) {
    // Off the main thread: stage only; the actual re-mint happens in RebuildStaged.
    g_staged.clear();

    if (version != kRecordVersion) {
        SKSE::log::warn("ProcgenItem: OnLoad record version {} != {} (ignored)", version,
                        kRecordVersion);
        return;
    }
    std::uint32_t count = 0;
    if (intfc->ReadRecordData(count) == 0) {
        SKSE::log::error("ProcgenItem: OnLoad failed to read count");
        return;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        StagedRecipe s;
        if (!ReadString(intfc, s.key)) break;
        std::uint32_t kindByte = 0;
        intfc->ReadRecordData(kindByte);
        s.kind = static_cast<Kind>(kindByte);
        RE::FormID storedTemplate = 0;
        intfc->ReadRecordData(storedTemplate);
        intfc->ReadRecordData(s.count);
        if (!ReadString(intfc, s.recipeJson)) break;

        // Remap the EXISTING plugin template FormID to this load order (research
        // §5 step 3 / SerializationInterface::ResolveFormID).
        RE::FormID resolved = storedTemplate;
        if (!intfc->ResolveFormID(storedTemplate, resolved)) {
            SKSE::log::warn("ProcgenItem: OnLoad ResolveFormID({:X}) failed; using as-is",
                            storedTemplate);
            resolved = storedTemplate;
        }
        s.templatePluginFormID = resolved;
        g_staged.push_back(std::move(s));
    }
    SKSE::log::info("ProcgenItem: OnLoad staged {} recipes for main-thread rebuild",
                    g_staged.size());
}

void OnRevert(SKSE::SerializationInterface*) {
    std::scoped_lock lock(g_mutex);
    Registry().clear();
    g_staged.clear();
    SKSE::log::info("ProcgenItem: OnRevert cleared the in-memory registry");
}

bool Register() {
    skyrim::cosave::AddHandler({ kRecordType, &OnSave, &OnLoad, &OnRevert });
    SKSE::log::info("ProcgenItem: registered 'GITM' co-save handler with dispatcher");
    return true;
}

// ---------------------------------------------------------------------------
// Rebuild-on-load — runs on the main thread (research §5 step 3).
// ---------------------------------------------------------------------------

void RebuildStaged() {
    std::vector<StagedRecipe> staged;
    staged.swap(g_staged);
    if (staged.empty()) return;

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::error("ProcgenItem: RebuildStaged has no player");
        return;
    }

    // PERSISTENCE NOTE (research §3.2 / ALCHEMY_SPIKE_FINDINGS §2): the vanilla
    // codec MAY have serialized the previous session's dynamic item base into the
    // player's inventory (as a real form, or a blank shell with form type only).
    // We treat the co-save recipe as the SOURCE OF TRUTH and re-mint fresh — but to
    // avoid the item ACCUMULATING by `count` on every reload, we strip the prior
    // restored instance (by recipe name + dynamic form type) BEFORE re-adding, so
    // each persist_key ends up with exactly one instance (header §35-38). A nameless
    // blank shell cannot be matched and is left alone (documented limitation).
    // The registry is cleared because the old session's minted-base pointers are gone.
    {
        std::scoped_lock lock(g_mutex);
        Registry().clear();
    }

    std::size_t rebuilt = 0;
    for (auto& s : staged) {
        nlohmann::json recipe;
        try {
            recipe = nlohmann::json::parse(s.recipeJson);
        } catch (const std::exception& e) {
            SKSE::log::error("ProcgenItem: RebuildStaged bad recipe for '{}': {}", s.key, e.what());
            continue;
        }

        RE::FormID tplFormID = s.templatePluginFormID;
        auto* tmpl = RE::TESForm::LookupByID(tplFormID);
        if (!tmpl) {
            // Fall back to resolving the recipe's "template" string (research §5 step 3).
            tmpl = ResolveTemplate(recipe.value("template", std::string{}), s.kind, tplFormID);
        }

        auto* obj = MintItem(tmpl, s.kind, recipe);
        if (!obj) {
            SKSE::log::warn("ProcgenItem: RebuildStaged failed to mint '{}'", s.key);
            continue;
        }
        std::int32_t count = s.count > 0 ? s.count : 1;
        // Strip the prior restored instance (the registry was cleared, so there is
        // no live pointer — the inventory scan by recipe name + dynamic form type
        // catches the codec-restored base) before re-adding, so it does not
        // accumulate by `count` on every reload (header §35-38, H1).
        StripPriorInstances(player, s.kind, recipe.value("name", std::string{}), nullptr, count);
        player->AddObjectToContainer(obj, nullptr, count, nullptr);

        TrackedItem t;
        t.key = s.key;
        t.kind = s.kind;
        t.templatePluginFormID = tplFormID;
        t.recipeJson = s.recipeJson;
        t.count = count;
        t.live = obj;
        {
            std::scoped_lock lock(g_mutex);
            Registry()[s.key] = std::move(t);
        }
        ++rebuilt;
        SKSE::log::info("ProcgenItem: rebuilt '{}' -> {} base {:X} x{}", s.key, KindName(s.kind),
                        obj->GetFormID(), count);
    }
    SKSE::log::info("ProcgenItem: RebuildStaged rebuilt {}/{} generated items", rebuilt,
                    staged.size());
}

std::size_t TrackedCount() {
    std::scoped_lock lock(g_mutex);
    return Registry().size();
}

}  // namespace skyrim::procgen::item
