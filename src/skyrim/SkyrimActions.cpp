#include "SkyrimActions.h"

#include "util.h"  // AnimUtil::Idle::Play (play_idle)

// >>> procgen: JSON-driven procedural-generation adapter actions.
#include "skyrim/procgen/Procgen.h"
// <<< procgen
// >>> gen-npc: JSON-driven runtime NPC form generation (research/PROCGEN_NPC_FORMS.md).
#include "skyrim/procgen/ProcgenNpc.h"
#include <filesystem>
#include <fstream>
// <<< gen-npc
// >>> gen-item: JSON-driven runtime ITEM (weapon/armor/misc) form generation.
#include "skyrim/procgen/ProcgenItem.h"
// <<< gen-item

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
        const std::string msg = subject.empty() ? "You have received a letter." : ("You have received a letter: " + subject);
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

    // >>> gen-item: runtime procedural ITEM form generation (mirrors generate_npc).
    // Skyrim adapter-extension action (SPEC §4.4) — mints a brand-new
    // TESObjectWEAP/ARMO/MISC base from a vanilla template + recipe, applies
    // overrides, adds it to the PLAYER's inventory, tracks it, and co-saves the
    // recipe so it rebuilds on reload (research/PROCGEN_NPC_FORMS.md form pattern +
    // ALCHEMY_SPIKE_FINDINGS persistence findings). No anchor needed — items go to
    // the player. Recipe source (same shape as generate_npc): inline "recipe"
    // object, OR a "recipe"/"template" JSON file name under config/procgen/, OR the
    // params themselves as the recipe.
    if (verb == "generate_item") {
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
                        SKSE::log::error("SkyrimActions: generate_item bad recipe {}: {}",
                                         path.string(), e.what());
                    }
                } else {
                    SKSE::log::warn("SkyrimActions: generate_item cannot open recipe {}",
                                    path.string());
                }
            } else if (params.is_object()) {
                recipe = params;
                ok = recipe.is_object();
            }
        }
        if (!ok) {
            SKSE::log::warn("SkyrimActions: generate_item skipped (no valid recipe)");
            return;
        }
        const std::string key = procgen::item::Generate(recipe);
        SKSE::log::info("SkyrimActions: generate_item key='{}'", key);
        return;
    }
    // <<< gen-item

    // ---- play_idle: play an animation on a character (DESIGN §2.2). Uses the
    // verified AnimUtil::Idle::Play wrapper (RELOCATION_ID(38290, 39256)); target
    // defaults to the player when no `character` alias is given. ----
    if (verb == "play_idle") {
        const std::string ref = params.value("form", std::string{});
        auto* idle = SkyrimEntities::resolveAs<RE::TESIdleForm>(ref);
        const std::string who = params.value("character", std::string{});
        RE::Actor* actor = who.empty() ? player : entities_.resolveCharacter(who);
        if (!idle || !actor) {
            SKSE::log::warn("SkyrimActions: play_idle skipped (idle '{}' / actor '{}')", ref, who);
            return;
        }
        const bool ok = AnimUtil::Idle::Play(idle, actor, RE::DEFAULT_OBJECT::kActionIdle, nullptr);
        SKSE::log::info("SkyrimActions: play_idle '{}' on '{}' -> {}",
                        ref, who.empty() ? "player" : who, ok);
        return;
    }

    // ---- add_shout: teach the player a shout (mirrors add_spell). ----
    if (verb == "add_shout") {
        const std::string ref = params.value("form", std::string{});
        auto* shout = SkyrimEntities::resolveAs<RE::TESShout>(ref);
        if (!player || !shout) {
            SKSE::log::warn("SkyrimActions: add_shout skipped (shout '{}' null)", ref);
            return;
        }
        player->AddShout(shout);
        SKSE::log::info("SkyrimActions: add_shout {}", ref);
        return;
    }

    // ---- teleport_player: move the player to a character alias or a placed ref
    // (TESObjectREFR::MoveTo handles the cell/worldspace transition). ----
    if (verb == "teleport_player") {
        const std::string to = params.value("to", std::string{});
        if (!player) {
            SKSE::log::warn("SkyrimActions: teleport_player skipped (no player)");
            return;
        }
        RE::TESObjectREFR* dest = entities_.resolveCharacter(to);
        if (!dest) {
            if (auto* form = SkyrimEntities::resolveForm(to)) dest = form->As<RE::TESObjectREFR>();
        }
        if (!dest) {
            SKSE::log::warn("SkyrimActions: teleport_player target '{}' unresolved", to);
            return;
        }
        player->MoveTo(dest);
        SKSE::log::info("SkyrimActions: teleport_player -> '{}'", to);
        return;
    }

    // ---- play_sound: play a BGSSoundDescriptorForm at the player (or a character).
    // BuildSoundDataFromDescriptor + BSSoundHandle::Play (BGSSoundDescriptorForm
    // publicly derives BSISoundDescriptor, so it upcasts to the param type). ----
    if (verb == "play_sound") {
        const std::string ref = params.value("form", std::string{});
        auto* sound = SkyrimEntities::resolveAs<RE::BGSSoundDescriptorForm>(ref);
        auto* audio = RE::BSAudioManager::GetSingleton();
        if (!sound || !audio) {
            SKSE::log::warn("SkyrimActions: play_sound skipped (sound '{}' / audio mgr null)", ref);
            return;
        }
        const std::string who = params.value("character", std::string{});
        RE::Actor* at = who.empty() ? player : entities_.resolveCharacter(who);
        RE::BSSoundHandle handle;
        if (audio->BuildSoundDataFromDescriptor(handle, sound) && handle.IsValid()) {
            if (at) handle.SetPosition(at->GetPosition());
            handle.Play();
            SKSE::log::info("SkyrimActions: play_sound {} at '{}'", ref, who.empty() ? "player" : who);
        } else {
            SKSE::log::warn("SkyrimActions: play_sound '{}' build/handle failed", ref);
        }
        return;
    }

    // ---- add_map_marker: reveal an existing map-marker ref (make it visible +
    // fast-travelable). Map markers are placed refs carrying ExtraMapMarker; we do
    // NOT mint new ones (no runtime API for that without an ESP). ----
    if (verb == "add_map_marker") {
        const std::string ref = params.value("form", std::string{});
        auto* form = SkyrimEntities::resolveForm(ref);
        auto* refr = form ? form->As<RE::TESObjectREFR>() : nullptr;
        auto* marker = refr ? refr->extraList.GetByType<RE::ExtraMapMarker>() : nullptr;
        if (!marker || !marker->mapData) {
            SKSE::log::warn("SkyrimActions: add_map_marker skipped ('{}' not a map-marker ref)", ref);
            return;
        }
        marker->mapData->SetVisible(true);
        marker->mapData->flags.set(RE::MapMarkerData::Flag::kCanTravelTo);
        SKSE::log::info("SkyrimActions: add_map_marker '{}' (visible + travelable)", ref);
        return;
    }

    // ---- recognised but deferred: no CommonLibSSE wrapper, and faking these would
    // need an address-library native we don't have (CLAUDE.md: offsets are found
    // manually — do NOT invent them). They no-op with a clear reason rather than
    // silently. start_combat: Actor exposes only Stop/Update/IsInCombat, not the
    // real Actor::StartCombat native. set_relationship: only TESNPC::relationships
    // (BGSRelationship*) exists; setting a player rank at runtime means minting +
    // linking BGSRelationship forms, which DESIGN flags as fragile for dynamic NPCs.
    if (verb == "start_combat" || verb == "set_relationship") {
        SKSE::log::warn("SkyrimActions: '{}' deferred — needs an address-library native "
                        "we don't have (no-op); params={}", verb, params.dump());
        return;
    }

    SKSE::log::warn("SkyrimActions: unknown verb '{}' (no-op) params={}", verb, params.dump());
}

}  // namespace skyrim
