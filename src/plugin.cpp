#include "log.h"
#include "NpcGenerator.h"
// >>> quest-engine Skyrim adapter (Phase 1)
#include "skyrim/SkyrimAdapter.h"
// <<< quest-engine Skyrim adapter
// >>> alchemy-spike
#include "alchemy_spike/AlchemySpike.h"
// <<< alchemy-spike
// >>> procgen: example procedural-generation spells + procgen co-save persistence.
#include "skyrim/procgen/Procgen.h"
#include "skyrim/procgen/ProcgenSpells.h"
// <<< procgen
// >>> gen-npc: runtime NPC form generation + co-save rebuild (research/PROCGEN_NPC_FORMS.md).
#include "skyrim/procgen/ProcgenNpc.h"
// <<< gen-npc
// >>> gen-item: runtime ITEM (weapon/armor/misc) form generation + 'GITM' co-save rebuild.
#include "skyrim/procgen/ProcgenItem.h"
// <<< gen-item
// >>> cosave: central SerializationInterface dispatcher (ONE SetUniqueID per plugin).
#include "skyrim/CoSave.h"
// <<< cosave

void OnDataLoaded()
{
    NpcGenerator::InitializeMagic();
    // >>> quest-engine Skyrim adapter (Phase 1): load + start the demo quest.
    skyrim::SkyrimAdapter::GetSingleton()->StartDemoQuest();
    // <<< quest-engine Skyrim adapter
    // >>> alchemy-spike: register the F11 debug brew trigger.
    AlchemySpike::Init();
    // <<< alchemy-spike
    // >>> procgen: create the dynamic Generate Room / Conjure Keep / Rearrange spells.
    skyrim::procgen::InitializeSpells();
    // <<< procgen
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		SKSE::log::info("kPostLoad: all SKSE plugins loaded");
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		SKSE::log::info("kDataLoaded: game data loaded, main menu ready");
		OnDataLoaded();
		break;
	case SKSE::MessagingInterface::kNewGame:
		SKSE::log::info("kNewGame: new game started");
		NpcGenerator::GiveSpellsToPlayer();
		// >>> procgen: dynamic spells aren't persisted, re-add on new game.
		skyrim::procgen::GiveSpellsToPlayer();
		// <<< procgen
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		SKSE::log::info("kPreLoadGame: save load starting");
		break;
	case SKSE::MessagingInterface::kPostLoadGame: {
		const bool success = a_msg->data != nullptr;
		SKSE::log::info("kPostLoadGame: save loaded (success={})", success);
		if (success) {
			NpcGenerator::GiveSpellsToPlayer();
			// >>> procgen: re-add dynamic spells after a save load.
			skyrim::procgen::GiveSpellsToPlayer();
			// <<< procgen
			// >>> gen-npc: rebuild co-saved NPCs on the main thread (research §5
			// "時機": form system ready, after OnLoad staged the recipes).
			skyrim::procgen::npc::RebuildStaged();
			// <<< gen-npc
				// >>> gen-item: rebuild co-saved items (re-mint + re-add to player) on
				// the main thread after OnLoad staged the 'GITM' recipes.
				skyrim::procgen::item::RebuildStaged();
				// <<< gen-item
			// >>> procgen-persist: rebuild co-saved rooms/structures on the main
			// thread from {recipe, origin, seed} (research §5.2 strategy B).
			skyrim::procgen::RebuildStaged();
			// <<< procgen-persist
			// >>> qe-persist: apply the staged 'QEST' progress blob (quest vars,
			// objectives, dialogue node, pending timers) + system globals onto the
			// engine built at kDataLoaded. Main-thread; no-op if nothing staged.
			skyrim::SkyrimAdapter::RebuildStaged();
			// <<< qe-persist
		}
		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			SKSE::log::info("  Player: {}", player->GetName());
		}
	} break;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
	SKSE::Init(skse);
	SetupLog();
	SKSE::log::info("Plugin loaded");

	auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		SKSE::log::error("Failed to register SKSE message listener");
		return false;
	}

	// >>> cosave: ONE SerializationInterface registration for the whole plugin.
	// SKSE allows only one SetUniqueID + one each Save/Load/Revert callback per
	// plugin (last wins, silently clobbering). So each module registers a record
	// handler with the central dispatcher (cosave::AddHandler) FIRST, then we
	// install the single SetUniqueID + callbacks via cosave::Register. Must happen
	// in SKSEPluginLoad after SKSE::Init; fenced + null-checked.
	skyrim::procgen::npc::Register();  // 'GNPC' handler (generated NPCs)
	skyrim::procgen::Register();       // 'PRGN' handler (generated rooms/structures)
	// >>> gen-item: 'GITM' handler (generated weapon/armor/misc items).
	skyrim::procgen::item::Register();
	// <<< gen-item
	// >>> qe-persist: 'QEST' handler (quest-engine progress + system globals, SPEC §6).
	skyrim::SkyrimAdapter::Register();
	// <<< qe-persist
	if (auto* serialization = SKSE::GetSerializationInterface()) {
		skyrim::cosave::Register(serialization);
	} else {
		SKSE::log::error("cosave: SerializationInterface unavailable; co-save disabled");
	}
	// <<< cosave

	return true;
}