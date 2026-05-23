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

# Portable quest-engine core (DESIGN §6). ZERO RE::/SKSE:: — kept out of the
# forced PCH (RE/Skyrim.h) in CMakeLists.txt via SKIP_PRECOMPILE_HEADERS so the
# core TUs stay game-agnostic.
set(core_sources
    src/core/QuestEngine.cpp
)
set(sources ${sources} ${core_sources})