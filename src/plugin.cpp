#include "log.h"
#include "NpcGenerator.h"
// >>> quest-engine Skyrim adapter (Phase 1)
#include "skyrim/SkyrimAdapter.h"
// <<< quest-engine Skyrim adapter
// >>> alchemy-spike
#include "alchemy_spike/AlchemySpike.h"
// <<< alchemy-spike
// >>> procgen: example procedural-generation spells.
#include "skyrim/procgen/ProcgenSpells.h"
// <<< procgen

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

	return true;
}