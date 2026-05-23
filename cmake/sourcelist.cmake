set(sources ${sources}
    src/plugin.cpp
    src/hook.cpp
    src/NpcGenerator.cpp
)

# Portable quest-engine core (DESIGN §6). ZERO RE::/SKSE:: — kept out of the
# forced PCH (RE/Skyrim.h) in CMakeLists.txt via SKIP_PRECOMPILE_HEADERS so the
# core TUs stay game-agnostic.
set(core_sources
    src/core/QuestEngine.cpp
)
set(sources ${sources} ${core_sources})