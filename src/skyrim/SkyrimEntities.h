#pragma once

// EntityResolver (SPEC §5.1, DESIGN §2.1) — Skyrim implementation.
//
// The portable core never resolves entity refs itself: it forwards extension
// actions/conditions/events to the adapter with their raw JSON params (e.g.
// {"character":"victim"}). This resolver turns the quest's `characters` aliases
// and those raw ref strings into live RE::Actor* / RE::TESObjectREFR*.
//
// Two binding kinds (DESIGN §2.1):
//   existing  {bind:"existing", ref:"<FormID~mod>" | "<EditorID>"}
//   spawn     {bind:"spawn", template:"<FormID~mod>"|"<0xID>", name:"<display>"}
//
// NOTE: this is NOT one of the QuestEngine::Deps ports — it is an adapter-private
// helper shared by SkyrimActions / SkyrimConditions / SkyrimEvents. Resolution
// happens on the main thread (callers already marshal via AddTask).

#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace skyrim {

class SkyrimEntities {
public:
    // Pre-bind the quest's `characters` block (called once at adapter start, on
    // the main thread). `existing` aliases are resolved eagerly; `spawn` aliases
    // are recorded as templates and instantiated on first spawn_character.
    void bindCharacters(const nlohmann::json& charactersBlock);

    // Resolve a character alias to a live actor. For `existing` it looks up the
    // bound form; for `spawn` it returns the already-spawned handle (or null if
    // not spawned yet). Returns null on any failure (SPEC §5.3: callers treat
    // null as "unevaluable / no-op").
    RE::Actor* resolveCharacter(const std::string& alias);

    // Spawn the `spawn`-bound alias near the player (NpcGenerator pattern), cache
    // and return it. `displayName` overrides the alias `name` if non-empty.
    RE::Actor* spawnCharacter(const std::string& alias, const std::string& displayName = "");

    // Reverse lookup: given a live ref's FormID, return the bound alias whose
    // resolved actor matches (or "" if none). Used by the EventSource to turn a
    // game event's object back into the JSON alias the triggers filter on.
    std::string aliasForFormID(RE::FormID id);

    // Resolve an arbitrary form ref string ("<FormID~mod>" or EditorID) to a form.
    static RE::TESForm* resolveForm(const std::string& ref);

    // Convenience: resolve a form ref string to a typed bound object (item/spell…).
    template <class T>
    static T* resolveAs(const std::string& ref) {
        auto* f = resolveForm(ref);
        return f ? f->As<T>() : nullptr;
    }

private:
    struct Binding {
        std::string bind;      // "existing" | "spawn"
        std::string ref;       // existing: form ref string / EditorID
        std::string templ;     // spawn: template form ref
        std::string name;      // spawn: display-name override
    };

    std::unordered_map<std::string, Binding> bindings_;
    std::unordered_map<std::string, RE::ActorHandle> resolved_;  // alias -> live actor
};

}  // namespace skyrim
