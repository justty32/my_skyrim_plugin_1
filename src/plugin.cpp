#include "log.h"
#include "MagicToolkit.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		SKSE::log::info("kPostLoad: all SKSE plugins loaded");
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		SKSE::log::info("kDataLoaded: game data loaded");
		MagicToolkit::InitializeMagic();
		break;
	case SKSE::MessagingInterface::kNewGame:
		SKSE::log::info("kNewGame");
		MagicToolkit::GiveSpellsToPlayer();
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		SKSE::log::info("kPreLoadGame");
		break;
	case SKSE::MessagingInterface::kPostLoadGame: {
		const bool success = a_msg->data != nullptr;
		SKSE::log::info("kPostLoadGame: success={}", success);
		if (success) {
			MagicToolkit::GiveSpellsToPlayer();
		}
	} break;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse);
	SetupLog();
	SKSE::log::info("MagicToolkit plugin loaded");

	auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		SKSE::log::error("Failed to register SKSE message listener");
		return false;
	}

	return true;
}
