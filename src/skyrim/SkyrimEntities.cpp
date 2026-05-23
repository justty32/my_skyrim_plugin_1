#include "SkyrimEntities.h"

#include "util.h"

namespace skyrim {

RE::TESForm* SkyrimEntities::resolveForm(const std::string& ref) {
    if (ref.empty()) return nullptr;

    // "<FormID>~<modName>" form (FormUtil::Parse, util.h). Contains a '~'.
    if (ref.find('~') != std::string::npos) {
        return FormUtil::Parse::GetFormFromConfigString(ref);
    }

    // Bare "0x..." / decimal id: treat as an absolute FormID (e.g. Skyrim.esm
    // base forms like Gold001 0x0000000F, the 0x7 NPC template).
    if (ref.size() > 1 && (ref[0] == '0') && (ref[1] == 'x' || ref[1] == 'X')) {
        try {
            const auto id = static_cast<RE::FormID>(std::stoul(ref, nullptr, 16));
            if (auto* f = RE::TESForm::LookupByID(id)) return f;
        } catch (...) {
        }
    }

    // Otherwise treat as an EditorID (NpcGenerator uses this pattern).
    return RE::TESForm::LookupByEditorID(ref);
}

void SkyrimEntities::bindCharacters(const nlohmann::json& charactersBlock) {
    bindings_.clear();
    resolved_.clear();
    if (!charactersBlock.is_object()) return;

    for (auto& [alias, desc] : charactersBlock.items()) {
        Binding b;
        b.bind = desc.value("bind", std::string{});
        b.ref = desc.value("ref", std::string{});
        b.templ = desc.value("template", std::string{});
        b.name = desc.value("name", std::string{});
        bindings_[alias] = b;

        if (b.bind == "existing") {
            // Resolve eagerly so resolution failures surface at start.
            if (auto* form = resolveForm(b.ref)) {
                if (auto* actor = form->As<RE::Actor>()) {
                    resolved_[alias] = actor->CreateRefHandle();
                    SKSE::log::info("EntityResolver: bound existing '{}' -> {:X}",
                                    alias, actor->GetFormID());
                    continue;
                }
            }
            SKSE::log::warn("EntityResolver: existing alias '{}' ref '{}' did not resolve to an Actor",
                            alias, b.ref);
        }
        // `spawn` bindings stay lazy until spawnCharacter().
    }
}

RE::Actor* SkyrimEntities::resolveCharacter(const std::string& alias) {
    auto it = resolved_.find(alias);
    if (it != resolved_.end()) {
        if (auto ptr = it->second.get()) return ptr.get();
        SKSE::log::warn("EntityResolver: alias '{}' handle is stale/invalid", alias);
    }
    SKSE::log::warn("EntityResolver: alias '{}' is not resolved", alias);
    return nullptr;
}

std::string SkyrimEntities::aliasForFormID(RE::FormID id) {
    for (auto& [alias, handle] : resolved_) {
        if (auto ptr = handle.get()) {
            if (ptr->GetFormID() == id) return alias;
        }
    }
    return {};
}

RE::Actor* SkyrimEntities::spawnCharacter(const std::string& alias, const std::string& displayName) {
    auto bit = bindings_.find(alias);
    if (bit == bindings_.end()) {
        // No `characters` binding for this alias (e.g. the demo's spawn_character
        // gives only a name). Synthesize an ad-hoc spawn binding off the vanilla
        // template so the action still produces a tracked NPC.
        Binding adhoc;
        adhoc.bind = "spawn";
        adhoc.name = displayName;
        bit = bindings_.emplace(alias, std::move(adhoc)).first;
        SKSE::log::info("EntityResolver: spawnCharacter '{}' unbound -> ad-hoc spawn binding", alias);
    }
    const Binding& b = bit->second;

    // Pick the template: explicit `template` in the binding, else fall back to
    // the vanilla generic NPC (0x7) like NpcGenerator does.
    RE::TESNPC* templateNPC = nullptr;
    if (!b.templ.empty()) {
        if (auto* f = resolveForm(b.templ)) templateNPC = f->As<RE::TESNPC>();
    }
    if (!templateNPC) {
        templateNPC = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);
    }
    auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
    if (!templateNPC || !factory) {
        SKSE::log::error("EntityResolver: spawn '{}' failed (no template/factory)", alias);
        return nullptr;
    }

    auto* newBase = factory->Create()->As<RE::TESNPC>();
    if (!newBase) {
        SKSE::log::error("EntityResolver: spawn '{}' failed (factory returned null)", alias);
        return nullptr;
    }
    newBase->Copy(templateNPC);
    const std::string finalName = !displayName.empty() ? displayName : b.name;
    if (!finalName.empty()) newBase->fullName = finalName;

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::error("EntityResolver: spawn '{}' failed (no player anchor)", alias);
        return nullptr;
    }

    // PlaceObjectAtMe at the player, then nudge in front (NpcGenerator pattern).
    auto spawned = player->PlaceObjectAtMe(newBase, false);
    auto* actor = spawned ? spawned->As<RE::Actor>() : nullptr;
    if (!actor) {
        SKSE::log::error("EntityResolver: spawn '{}' PlaceObjectAtMe failed", alias);
        return nullptr;
    }

    const float angleZ = player->data.angle.z;
    RE::NiPoint3 pos = player->GetPosition();
    pos.x += std::sin(angleZ) * 150.0f;
    pos.y += std::cos(angleZ) * 150.0f;
    pos.z += 10.0f;
    actor->SetPosition(pos, true);  // Actor::SetPosition takes (pos, updateCharController)
    actor->SetAngle(player->data.angle);

    resolved_[alias] = actor->CreateRefHandle();
    SKSE::log::info("EntityResolver: spawned '{}' ('{}') -> {:X}", alias, finalName,
                    actor->GetFormID());
    return actor;
}

}  // namespace skyrim
