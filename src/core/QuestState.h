#pragma once

// Portable quest-engine runtime state. ZERO game (RE::/SKSE::) dependencies —
// only the C++ standard library. See QUEST_ENGINE_SPEC.md.

#include <string>
#include <unordered_map>
#include <variant>

namespace qe {

// SPEC §2: a variable value is number / bool / string. Numbers are doubles.
using Value = std::variant<double, bool, std::string>;

// Same-type equality (SPEC §4.1 var_eq compares same type). std::variant's
// operator== is false across different alternatives, which is what we want.
inline bool valueEq(const Value& a, const Value& b) { return a == b; }

// Coerce to double for numeric ops / comparisons (bool → 0/1, string → 0).
inline double asDouble(const Value& v) {
    if (auto p = std::get_if<double>(&v)) return *p;
    if (auto b = std::get_if<bool>(&v)) return *b ? 1.0 : 0.0;
    return 0.0;
}

inline std::string valueToString(const Value& v) {
    return std::visit([](auto&& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return x;
        } else if constexpr (std::is_same_v<T, bool>) {
            return x ? "true" : "false";
        } else {  // double
            if (x == static_cast<long long>(x))
                return std::to_string(static_cast<long long>(x));
            return std::to_string(x);
        }
    }, v);
}

// Per-quest runtime state (SPEC §6 progress; persistence itself is Phase 1).
struct QuestState {
    std::unordered_map<std::string, Value> vars;          // quest-scoped vars
    std::unordered_map<std::string, std::string> objectives;  // id -> state string
    std::string activeDialogue;  // "" = no dialogue in progress
    std::string currentNode;     // "" = none
    bool terminated = false;     // complete_quest / fail_quest reached
};

// System-level store shared across quests (SPEC §2.4 global.* variables).
// In Phase 1 this is persisted at the system level in the co-save, not per quest.
struct GlobalStore {
    std::unordered_map<std::string, Value> vars;
};

}  // namespace qe
