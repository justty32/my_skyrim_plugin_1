#include "hook.h"
#include "FollowLight.h"
// SKSE::log via PCH (do not include log.h — it defines SetupLog()).
// (AmbientBoost is one-shot on keypress — no per-frame hook needed.)

namespace Hooks
{
    // Per-frame driver for FollowLight: hook PlayerCharacter's Update (Actor vfunc 0xAD), call the
    // original, then reposition the follow-light. A vtable write (not a code trampoline), so no
    // SKSE::AllocTrampoline needed. Runs on the main thread every frame the player updates.
    struct PlayerUpdateHook
    {
        static void thunk(RE::PlayerCharacter* a_player, float a_delta)
        {
            func(a_player, a_delta);
            FollowLight::Update();
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static constexpr std::size_t                    idx = 0xAD;  // Actor::Update(float)
    };

    void Install()
    {
        REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
        PlayerUpdateHook::func = vtbl.write_vfunc(PlayerUpdateHook::idx, PlayerUpdateHook::thunk);
        SKSE::log::info("Hooks: PlayerCharacter::Update hooked (FollowLight per-frame)");
    }
}
