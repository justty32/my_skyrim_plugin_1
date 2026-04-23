#include "log.h"
#include "shouts/power_shout.h"


void OnDataLoaded()
{
	PowerShout::Initialize();
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
		PowerShout::EnsureGrantedToPlayer();
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		SKSE::log::info("kPreLoadGame: save load starting");
		break;
	case SKSE::MessagingInterface::kPostLoadGame: {
		const bool success = a_msg->data != nullptr;
		SKSE::log::info("kPostLoadGame: save loaded (success={})", success);
		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			SKSE::log::info("  Player: {}", player->GetName());
		}
		if (success) {
			PowerShout::EnsureGrantedToPlayer();
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