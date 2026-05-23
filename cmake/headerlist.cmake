set(headers ${headers}
	src/PCH.h 
    src/log.h
    src/util.h
    src/hook.h 
    src/settings.h
    src/NpcGenerator.h
    src/core/Ports.h
    src/core/QuestEngine.h
    src/core/QuestState.h
)

# >>> quest-engine Skyrim adapter (Phase 1, DESIGN §6). Appended for trivial merge.
set(headers ${headers}
    src/skyrim/SkyrimAdapter.h
    src/skyrim/SkyrimEntities.h
    src/skyrim/SkyrimActions.h
    src/skyrim/SkyrimConditions.h
    src/skyrim/SkyrimEvents.h
    src/skyrim/dialogue/MessageBoxPresenter.h
)
# <<< quest-engine Skyrim adapter

# >>> alchemy-spike
set(headers ${headers}
    src/alchemy_spike/AlchemySpike.h
)
# <<< alchemy-spike