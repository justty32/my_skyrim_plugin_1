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

# >>> procgen (PROCGEN_INTERIOR/EXTERIOR.md) — procedural placement module headers.
set(headers ${headers}
    src/skyrim/procgen/Procgen.h
    src/skyrim/procgen/ProcgenSpells.h
)
# <<< procgen

# >>> gen-npc (research/PROCGEN_NPC_FORMS.md) — runtime NPC form generation header.
set(headers ${headers}
    src/skyrim/procgen/ProcgenNpc.h
)
# <<< gen-npc

# >>> gen-item — runtime ITEM (weapon/armor/misc) form generation header.
set(headers ${headers}
    src/skyrim/procgen/ProcgenItem.h
)
# <<< gen-item

# >>> cosave — central SKSE co-save dispatcher header.
set(headers ${headers}
    src/skyrim/CoSave.h
)
# <<< cosave

# >>> native-dialogue-spike (DESIGN §5) — native NPC dialogue injection probe.
set(headers ${headers}
    src/skyrim/dialogue/NativeDialogueSpike.h
)
# <<< native-dialogue-spike