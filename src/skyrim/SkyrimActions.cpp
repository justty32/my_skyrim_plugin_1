#include "SkyrimActions.h"

namespace skyrim {

namespace {
constexpr RE::FormID kGold001 = 0x0000000F;  // Skyrim.esm Gold001

std::int32_t intParam(const nlohmann::json& p, const char* field, std::int32_t fallback) {
    if (p.is_object() && p.contains(field) && p[field].is_number())
        return p[field].get<std::int32_t>();
    return fallback;
}

RE::TESBoundObject* goldForm() {
    return RE::TESForm::LookupByID<RE::TESBoundObject>(kGold001);
}
}  // namespace

void SkyrimActions::run(const std::string& verb, const nlohmann::json& params) {
    auto* player = RE::PlayerCharacter::GetSingleton();

    // ---- gold ----
    if (verb == "give_gold" || verb == "remove_gold") {
        const std::int32_t amount = intParam(params, "amount", 0);
        auto* gold = goldForm();
        if (!player || !gold || amount <= 0) {
            SKSE::log::warn("SkyrimActions: {} skipped (player/gold null or amount<=0)", verb);
            return;
        }
        if (verb == "give_gold") {
            player->AddObjectToContainer(gold, nullptr, amount, nullptr);
        } else {
            player->RemoveItem(gold, amount, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
        }
        SKSE::log::info("SkyrimActions: {} {}", verb, amount);
        return;
    }

    // ---- items ----
    if (verb == "give_item" || verb == "remove_item") {
        const std::string ref = params.value("form", std::string{});
        const std::int32_t count = intParam(params, "count", 1);
        auto* obj = SkyrimEntities::resolveAs<RE::TESBoundObject>(ref);
        if (!player || !obj || count <= 0) {
            SKSE::log::warn("SkyrimActions: {} skipped (form '{}' / count {})", verb, ref, count);
            return;
        }
        if (verb == "give_item") {
            player->AddObjectToContainer(obj, nullptr, count, nullptr);
        } else {
            player->RemoveItem(obj, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
        }
        SKSE::log::info("SkyrimActions: {} {} x{}", verb, ref, count);
        return;
    }

    // ---- spells ----
    if (verb == "add_spell" || verb == "remove_spell") {
        const std::string ref = params.value("form", std::string{});
        auto* spell = SkyrimEntities::resolveAs<RE::SpellItem>(ref);
        if (!player || !spell) {
            SKSE::log::warn("SkyrimActions: {} skipped (spell '{}' null)", verb, ref);
            return;
        }
        if (verb == "add_spell") {
            player->AddSpell(spell);
        } else {
            player->RemoveSpell(spell);
        }
        SKSE::log::info("SkyrimActions: {} {}", verb, ref);
        return;
    }

    // ---- spawn / move characters (EntityResolver, NpcGenerator pattern) ----
    if (verb == "spawn_character") {
        const std::string alias = params.value("character", std::string{});
        const std::string name = params.value("name", std::string{});
        // The demo's spawn_character has only a name and relies on the alias's
        // spawn binding; if no `character` key, fall back to a conventional alias.
        const std::string useAlias = alias.empty() ? "victim" : alias;
        entities_.spawnCharacter(useAlias, name);
        return;
    }
    if (verb == "move_character") {
        const std::string alias = params.value("character", std::string{});
        const std::string to = params.value("to", std::string{});
        auto* actor = entities_.resolveCharacter(alias);
        if (!actor) {
            SKSE::log::warn("SkyrimActions: move_character '{}' unresolved", alias);
            return;
        }
        if (to == "player" || to.empty()) {
            if (player) actor->MoveTo(player);
        } else if (auto* target = entities_.resolveCharacter(to)) {
            actor->MoveTo(target);
        } else if (auto* ref = SkyrimEntities::resolveForm(to)) {
            if (auto* refr = ref->As<RE::TESObjectREFR>()) actor->MoveTo(refr);
        } else {
            SKSE::log::warn("SkyrimActions: move_character target '{}' unresolved", to);
            return;
        }
        SKSE::log::info("SkyrimActions: move_character '{}' -> '{}'", alias, to);
        return;
    }

    // ---- deliver_letter (SPEC §4.5 deliver_message). Minimal/safe: courier hook
    // is high-risk (progress.md open issue), so we give the player a BGSNote if a
    // `form` is supplied, else just a HUD notification, then leave message_ack to
    // a later letter_read EventSource. ----
    if (verb == "deliver_letter") {
        const std::string subject = params.value("subject", std::string{});
        const std::string ref = params.value("form", std::string{});
        if (player && !ref.empty()) {
            if (auto* note = SkyrimEntities::resolveAs<RE::TESBoundObject>(ref)) {
                player->AddObjectToContainer(note, nullptr, 1, nullptr);
                SKSE::log::info("SkyrimActions: deliver_letter gave note '{}'", ref);
            }
        }
        const std::string msg = subject.empty() ? "你收到一封信。" : ("你收到一封信：" + subject);
        RE::DebugNotification(msg.c_str());
        SKSE::log::info("SkyrimActions: deliver_letter subject='{}' (courier deferred)", subject);
        return;
    }

    // ---- recognised-but-deferred / high-risk verbs (DESIGN §2.2) ----
    if (verb == "start_combat" || verb == "add_shout" || verb == "teleport_player" ||
        verb == "set_relationship" || verb == "play_idle" || verb == "add_map_marker" ||
        verb == "play_sound") {
        SKSE::log::info("SkyrimActions: TODO not implemented '{}' (no-op) params={}",
                        verb, params.dump());
        return;
    }

    SKSE::log::warn("SkyrimActions: unknown verb '{}' (no-op) params={}", verb, params.dump());
}

}  // namespace skyrim
