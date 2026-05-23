#include "SkyrimActions.h"

// >>> procgen: JSON-driven procedural-generation adapter actions.
#include "skyrim/procgen/Procgen.h"
// <<< procgen
// >>> gen-npc: JSON-driven runtime NPC form generation (research/PROCGEN_NPC_FORMS.md).
#include "skyrim/procgen/ProcgenNpc.h"
#include <filesystem>
#include <fstream>
// <<< gen-npc

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

    // >>> procgen: generate_interior / generate_structure / rearrange_furnishings
    // (research/PROCGEN_INTERIOR.md §8, PROCGEN_EXTERIOR.md §9). These are Skyrim
    // adapter-extension actions (SPEC §4.4): the core does not understand them and
    // forwards verb+params here. Same Procgen module backs the example spells.
    //
    // Anchor: `at.ref` ("player" | a bound character alias | a form ref); defaults
    // to the player. Recipe: a "template" file under config/procgen/, OR an inline
    // recipe object/array under "recipe". `persist_key` tags the tracked refs.
    if (verb == "generate_interior" || verb == "generate_structure") {
        // Resolve the placement anchor.
        RE::TESObjectREFR* anchor = player;
        if (params.is_object() && params.contains("at")) {
            const std::string ref = params["at"].is_object()
                                        ? params["at"].value("ref", std::string{})
                                        : params["at"].get<std::string>();
            if (!ref.empty() && ref != "player") {
                if (auto* actor = entities_.resolveCharacter(ref)) {
                    anchor = actor;
                } else if (auto* form = SkyrimEntities::resolveForm(ref)) {
                    if (auto* refr = form->As<RE::TESObjectREFR>()) anchor = refr;
                }
            }
        }
        if (!anchor) {
            SKSE::log::warn("SkyrimActions: {} skipped (no anchor / player)", verb);
            return;
        }

        // Load the recipe: a template file name, or an inline recipe doc.
        procgen::Recipe recipe;
        nlohmann::json clutterDoc = nlohmann::json::array();
        bool ok = false;
        if (params.is_object() && params.contains("template") && params["template"].is_string()) {
            std::string file = params["template"].get<std::string>();
            if (file.find(".json") == std::string::npos) file += ".json";  // allow bare id
            ok = procgen::LoadRecipeFile(file, recipe);
            // For file-driven recipes we cannot re-read the raw clutter array, so
            // scatter falls back to authored positions (logged in Procgen).
        } else if (params.is_object() && params.contains("recipe")) {
            ok = procgen::ParseRecipe(params["recipe"], recipe);
            if (params["recipe"].is_object() && params["recipe"].contains("clutter")) {
                clutterDoc = params["recipe"]["clutter"];
            }
        }
        if (!ok) {
            SKSE::log::warn("SkyrimActions: {} skipped (no valid 'template'/'recipe')", verb);
            return;
        }

        const std::string key = params.is_object()
                                    ? params.value("persist_key", recipe.templateId)
                                    : recipe.templateId;
        if (verb == "generate_interior") {
            procgen::GenerateInterior(recipe, anchor, key, clutterDoc);
        } else {
            procgen::GenerateStructure(recipe, anchor, key);
        }
        SKSE::log::info("SkyrimActions: {} template='{}' key='{}'", verb, recipe.templateId, key);
        return;
    }
    if (verb == "rearrange_furnishings") {
        const std::string key = params.is_object()
                                    ? params.value("persist_key", std::string{})
                                    : std::string{};
        procgen::RearrangeFurnishings(key);
        SKSE::log::info("SkyrimActions: rearrange_furnishings key='{}'", key);
        return;
    }
    // <<< procgen

    // >>> gen-npc: runtime procedural NPC form generation with co-save rebuild
    // (research/PROCGEN_NPC_FORMS.md §1/§5/§11). Skyrim adapter-extension action
    // (SPEC §4.4) — the richer sibling of spawn_character: it mints a brand-new
    // TESNPC_ base from a template + recipe, places a ref near an anchor, tracks
    // it, and co-saves the recipe so it rebuilds on reload.
    //
    // Recipe source (mirrors generate_interior): an inline "recipe" object, OR a
    // "recipe"/"template" file name under config/procgen/. Anchor: `at.ref`
    // ("player" | a character alias | a form ref); defaults to the player.
    if (verb == "generate_npc") {
        // Resolve the placement anchor (same shape as generate_interior).
        RE::TESObjectREFR* anchor = player;
        if (params.is_object() && params.contains("at")) {
            const std::string ref = params["at"].is_object()
                                        ? params["at"].value("ref", std::string{})
                                        : params["at"].get<std::string>();
            if (!ref.empty() && ref != "player") {
                if (auto* actor = entities_.resolveCharacter(ref)) {
                    anchor = actor;
                } else if (auto* form = SkyrimEntities::resolveForm(ref)) {
                    if (auto* refr = form->As<RE::TESObjectREFR>()) anchor = refr;
                }
            }
        }
        if (!anchor) {
            SKSE::log::warn("SkyrimActions: generate_npc skipped (no anchor / player)");
            return;
        }

        // Recipe: inline object, else load a JSON file from config/procgen/.
        nlohmann::json recipe;
        bool ok = false;
        if (params.is_object() && params.contains("recipe") && params["recipe"].is_object()) {
            recipe = params["recipe"];
            ok = true;
        } else {
            std::string file;
            if (params.is_object() && params.contains("recipe") && params["recipe"].is_string()) {
                file = params["recipe"].get<std::string>();
            } else if (params.is_object() && params.contains("template") &&
                       params["template"].is_string() &&
                       params["template"].get<std::string>().find(".json") != std::string::npos) {
                file = params["template"].get<std::string>();
            }
            if (!file.empty()) {
                if (file.find(".json") == std::string::npos) file += ".json";
                namespace fs = std::filesystem;
                fs::path path = fs::path("Data") / "SKSE" / "Plugins" / "Template_Plugin" /
                                "procgen" / file;
                if (!fs::exists(path)) {
                    path = fs::path("Data") / "SKSE" / "Plugins" / "procgen" / file;
                }
                std::ifstream in(path);
                if (in) {
                    try {
                        in >> recipe;
                        ok = recipe.is_object();
                    } catch (const std::exception& e) {
                        SKSE::log::error("SkyrimActions: generate_npc bad recipe {}: {}",
                                         path.string(), e.what());
                    }
                } else {
                    SKSE::log::warn("SkyrimActions: generate_npc cannot open recipe {}",
                                    path.string());
                }
            } else if (params.is_object()) {
                // No "recipe"/"template" -> treat the params themselves as the
                // recipe (e.g. {"verb":"generate_npc","name":"...","template":"0x.."}).
                recipe = params;
                ok = recipe.is_object();
            }
        }
        if (!ok) {
            SKSE::log::warn("SkyrimActions: generate_npc skipped (no valid recipe)");
            return;
        }

        const std::string key = procgen::npc::Generate(recipe, anchor);
        SKSE::log::info("SkyrimActions: generate_npc key='{}'", key);
        return;
    }
    // <<< gen-npc

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
