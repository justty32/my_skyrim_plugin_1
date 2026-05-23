#pragma once

// SkyrimConditions (DESIGN §2.3) — ConditionEvaluator port (qe::IConditionEvaluator,
// SPEC §5.3). Evaluates adapter-extension condition keys against live game state.
// Unknown / unevaluable keys return false (SPEC §5.3) rather than aborting.
//
// Implemented (safe, read-only): player_level_gte / player_level_lte,
// player_gold_gte, player_has_item, player_has_spell.
// Other DESIGN §2.3 keys (character_alive/_dead, in_location, time_of_day) are
// recognised but left as clearly-logged TODO no-ops returning false.

#include "core/Ports.h"
#include "SkyrimEntities.h"

namespace skyrim {

class SkyrimConditions : public qe::IConditionEvaluator {
public:
    explicit SkyrimConditions(SkyrimEntities& entities) : entities_(entities) {}

    bool evaluate(const std::string& key, const nlohmann::json& params) override;

private:
    SkyrimEntities& entities_;
};

}  // namespace skyrim
