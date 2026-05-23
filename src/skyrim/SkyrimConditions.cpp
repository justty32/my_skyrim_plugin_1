#include "SkyrimConditions.h"

namespace skyrim {

namespace {
// JSON params can be a bare number ({"player_level_gte": 10}) or an object
// ({"player_level_gte": {"value": 10}}). Accept both for author/LLM friendliness.
double numberParam(const nlohmann::json& p, const char* field = "value") {
    if (p.is_number()) return p.get<double>();
    if (p.is_object() && p.contains(field) && p[field].is_number())
        return p[field].get<double>();
    return 0.0;
}
}  // namespace

bool SkyrimConditions::evaluate(const std::string& key, const nlohmann::json& params) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("SkyrimConditions: no player -> '{}' false", key);
        return false;
    }

    if (key == "player_level_gte") {
        return static_cast<double>(player->GetLevel()) >= numberParam(params);
    }
    if (key == "player_level_lte") {
        return static_cast<double>(player->GetLevel()) <= numberParam(params);
    }
    if (key == "player_gold_gte") {
        return static_cast<double>(player->GetGoldAmount()) >= numberParam(params);
    }
    if (key == "player_has_item") {
        const std::string ref = params.value("form", std::string{});
        const std::int32_t want = static_cast<std::int32_t>(numberParam(params, "count"));
        auto* obj = SkyrimEntities::resolveAs<RE::TESBoundObject>(ref);
        if (!obj) {
            SKSE::log::warn("SkyrimConditions: player_has_item form '{}' not found", ref);
            return false;
        }
        const std::int32_t have = player->GetItemCount(obj);
        return have >= (want > 0 ? want : 1);
    }
    if (key == "player_has_spell") {
        const std::string ref = params.value("form", std::string{});
        auto* spell = SkyrimEntities::resolveAs<RE::SpellItem>(ref);
        if (!spell) {
            SKSE::log::warn("SkyrimConditions: player_has_spell form '{}' not found", ref);
            return false;
        }
        return player->HasSpell(spell);
    }

    // Recognised-but-deferred DESIGN §2.3 keys: log and return false (SPEC §5.3).
    if (key == "character_alive" || key == "character_dead" ||
        key == "in_location" || key == "time_of_day") {
        SKSE::log::info("SkyrimConditions: TODO not implemented '{}' -> false", key);
        return false;
    }

    SKSE::log::warn("SkyrimConditions: unknown condition '{}' -> false", key);
    return false;
}

}  // namespace skyrim
