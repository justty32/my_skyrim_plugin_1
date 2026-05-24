set(sources ${sources}
    src/plugin.cpp
    src/hook.cpp
    src/NpcGenerator.cpp
)

# >>> quest-engine Skyrim adapter (Phase 1, DESIGN §6) — normal Skyrim TUs (use
# the PCH; do NOT mark SKIP_PRECOMPILE_HEADERS). Appended so a merge stays trivial.
set(sources ${sources}
    src/skyrim/SkyrimAdapter.cpp
    src/skyrim/SkyrimEntities.cpp
    src/skyrim/SkyrimActions.cpp
    src/skyrim/SkyrimConditions.cpp
    src/skyrim/SkyrimEvents.cpp
    src/skyrim/dialogue/MessageBoxPresenter.cpp
)
# <<< quest-engine Skyrim adapter

# >>> alchemy-spike (3D-physical-alchemy feasibility spike; F11 debug trigger)
set(sources ${sources}
    src/alchemy_spike/AlchemySpike.cpp
)
# <<< alchemy-spike

# >>> procgen (PROCGEN_INTERIOR/EXTERIOR.md) — procedural placement module: the
# Procgen core + the example spells. Normal Skyrim TUs (use the PCH). Appended so
# a merge stays trivial.
set(sources ${sources}
    src/skyrim/procgen/Procgen.cpp
    src/skyrim/procgen/ProcgenSpells.cpp
)
# <<< procgen

# >>> gen-npc (research/PROCGEN_NPC_FORMS.md) — runtime NPC form generation +
# co-save rebuild. Normal Skyrim TU (uses the PCH). Appended for a trivial merge.
set(sources ${sources}
    src/skyrim/procgen/ProcgenNpc.cpp
)
# <<< gen-npc

# >>> gen-item (research/PROCGEN_NPC_FORMS.md form-mint pattern + ALCHEMY_SPIKE
# persistence findings) — runtime ITEM (weapon/armor/misc) form generation +
# 'GITM' co-save rebuild. Normal Skyrim TU (uses the PCH). Appended for trivial merge.
set(sources ${sources}
    src/skyrim/procgen/ProcgenItem.cpp
)
# <<< gen-item

# >>> cosave — central SKSE co-save (SerializationInterface) dispatcher: ONE
# SetUniqueID + 3 callbacks per plugin, fanned out to per-record-type handlers
# ('GNPC', 'PRGN', ...). Normal Skyrim TU (uses the PCH). Appended for trivial merge.
set(sources ${sources}
    src/skyrim/CoSave.cpp
)
# <<< cosave

# >>> native-dialogue-spike (DESIGN §5) — Debug R&D probe for injecting a custom
# topic into the native NPC Dialogue Menu. Self-contained; remove this block + the
# OnDataLoaded() call to drop it. Normal Skyrim TU (uses the PCH).
set(sources ${sources}
    src/skyrim/dialogue/NativeDialogueSpike.cpp
)
# <<< native-dialogue-spike

# Portable quest-engine core (DESIGN §6). ZERO RE::/SKSE:: — kept out of the
# forced PCH (RE/Skyrim.h) in CMakeLists.txt via SKIP_PRECOMPILE_HEADERS so the
# core TUs stay game-agnostic.
set(core_sources
    src/core/QuestEngine.cpp
)
set(sources ${sources} ${core_sources})