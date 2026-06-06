# SKSE Plugin Development Pitfalls & Solutions

## 1. Initialization Timing & Data Reliability
- **The Pitfall:** Accessing `TESDataHandler` or performing `LookupByID` inside `kDataLoaded` can sometimes fail for standard ESM forms (like Trees/Rocks) in certain environments (Proton/VR).
- **The Symptom:** `nullptr` returned for forms that definitely exist in `Skyrim.esm`.
- **The Solution:** Use **Lazy Loading**. Instead of pre-caching forms during plugin load, perform the lookup the first time the resource is actually needed (e.g., inside the spell cast event).

## 2. Resource Lookup: ID vs. EditorID
- **The Pitfall:** Hardcoded Hex FormIDs (e.g., `0x38432`) are brittle. Type mismatches during casting (`As<T>`) can fail silently if the form type is not exactly what you expect.
- **The Symptom:** Logic skipped or errors logged during resource retrieval.
- **The Solution:** Prefer **`RE::TESForm::LookupByEditorID<RE::TESBoundObject>("EditorID")`**. It is much more robust across different game versions and mod setups.

## 3. The "Main Menu CTD" (Null Pointer Dereference)
- **The Pitfall:** Calling member functions on a lookup result without checking for null: `LookupByID(ID)->As<T>()`.
- **The Symptom:** Game crashes immediately after the Bethesda logo or during the loading screen.
- **The Solution:** Always use defensive null checks.
  ```cpp
  auto* form = RE::TESForm::LookupByID(ID);
  auto* boundObj = form ? form->As<RE::TESBoundObject>() : nullptr;
  if (boundObj) { ... }
  ```

## 4. Spawning Objects: `PlaceAtMe` is King
- **The Pitfall:** Manually creating a `RE::TESObjectREFR` and trying to initialize its 3D.
- **The Symptom:** Objects are invisible, have no collision, or don't appear in the world at all.
- **The Solution:** Use **`RE::TESObjectREFR::PlaceObjectAtMe(baseForm, isInitiallyDisabled)`**. This internal engine API handles reference creation, 3D initialization, and cell attachment correctly.

## 5. C++ API & Smart Pointers (`NiPointer`)
- **The Pitfall:** Assigning the result of `PlaceObjectAtMe` to an `auto*` raw pointer.
- **The Symptom:** Compilation error: `incompatible initializer of type 'NiPointer<TESObjectREFR>'`.
- **The Solution:** Use `auto` to let the compiler deduce `RE::NiPointer<T>`, or use `.get()` if you specifically need the raw pointer.

## 6. Physics Collisions & Coordinate Math
- **The Pitfall:** Spawning objects at the exact `GetPosition()` of the player.
- **The Symptom:** Player gets stuck inside the object, or physics "explosions" occur.
- **The Solution:** Offset the spawn position using the player's rotation (`angle.z`).
  ```cpp
  float angleZ = player->data.angle.z;
  RE::NiPoint3 forward(std::sin(angleZ), std::cos(angleZ), 0.0f);
  RE::NiPoint3 spawnPos = player->GetPosition() + (forward * 200.0f);
  ```

## 7. Dynamic Form Persistence (0xFF Range)
- **The Pitfall:** Comparing pointers or using FormIDs to detect spells/items created dynamically at runtime.
- **The Symptom:** Spells stop working after saving and loading the game because dynamic FormIDs can change.
- **The Solution:** Use **Name Matching**. Compare `fullName` (e.g., `"C++: Place Tree"`) to identify your custom forms reliably after a save/load cycle.

## 8. Runtime Lights: do NOT poke `ShadowSceneNode` / attach to the player skeleton
- **The Pitfall:** Implementing a dynamic light by creating a `RE::NiPointLight`, `AttachChild`-ing it to the player's 3D (the animated skeleton `BSFadeNode`), and registering it with `RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0]->AddLight(...)` — i.e. mutating the engine's scene graph + light list yourself. (This is what the Lighting Toolkit photography mod does; it works there because the scene is static during screenshots.)
- **The Symptom:** Intermittent `EXCEPTION_ACCESS_VIOLATION` (null vtable call, `call [rax+0x..]`, rax=0) **on a `BSJobs::JobThread` or the `dxvk-cs` render thread**, inside `BSGeometryListCullingProcess` / `BSShaderAccumulator` / the animation graph update — with `ShadowSceneNode` and a `skeleton.nif` `BSFadeNode` in the object chain. Triggers grow with concurrency: fine in quiet interiors, **crashes when spamming the toggle, on cell/map transitions, and reliably in busy exteriors** (many NPCs animating). Worse on Proton (threaded renderer). The `SKSE::GetTaskInterface()` does NOT save you — it only syncs with the main thread, while the engine's lighting/culling/animation **worker jobs run concurrently** and read the same light list / skeleton tree you're mutating.
- **The Solution:** **Let the engine own the light.** Author the light as a `LIGH` base form (a no-shadow, large-radius "bulb"; tiered copies for brightness levels — easy to generate with an ESP/ESL) and `player->PlaceObjectAtMe(ligh, false)` it. The engine's normal cell/lighting pipeline handles rendering, culling, cross-cell, and threading — internally synchronized, so it never races. Toggle off with `ref->Disable(); ref->SetDelete(true);`. Hold the ref via an `ObjectRefHandle` (survives the engine cleaning it on cell unload). Look it up with `TESDataHandler::LookupForm<RE::TESObjectLIGH>(localID, "YourPlugin.esp")`.
  - A real light (vs. an ImageSpace / screen post-process) is also the *only* way to brighten shadowed geometry **without washing out low-albedo things like black armor**: ambient/lighting is modulated by material albedo + normals; a screen filter is a uniform multiply that can't tell a shadow from a black object.
  - Even with `PlaceObjectAtMe`, still **debounce** the hotkey and **coalesce** bursts (only one apply queued at a time) — rapid place/delete churn is wasteful and unnecessary.
  - See `src/FollowLight.cpp`. Background analysis: `research/cell-ambient-feasibility.md`.

## 9. Per-frame work: hook PlayerCharacter::Update — do NOT self-reschedule a task
- **The Pitfall:** Driving a per-frame action by having a `SKSE::GetTaskInterface()->AddTask` callback re-add itself each time ("self-rescheduling task loop").
- **The Symptom:** The whole game **freezes** (main thread locked) the instant the loop starts — not a crash, a hard hang.
- **The Solution:** Use a real per-frame hook. Write the player's `Update` vfunc: `Actor::Update(float)` is **vfunc 0xAD**; `REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] }; func = vtbl.write_vfunc(0xAD, thunk);` — call the original in the thunk, then do your per-frame work. It's a vtable write (no trampoline / `AllocTrampoline` needed), runs on the main thread every frame. See `src/hook.cpp` (drives `FollowLight::Update`).

## 10. Runtime cell ambient: write INTERIOR_DATA *and clear the LGTM inherit flags*
- **The Pitfall:** Trying to brighten a whole interior cell by either (a) per-frame writing `Sky::directionalAmbientColors` (gets overwritten by the engine's later lighting pass — no effect), or (b) writing `cell->GetLighting()->ambient` while leaving the lighting-template inheritance flags set (the cell value is ignored — the LGTM value wins).
- **The Symptom:** No visible change.
- **The Solution:** On the cell's `RE::INTERIOR_DATA` (`cell->GetLighting()`, interior only): lift `ambient` + `directional` + the 6 `directionalAmbientLightingColors` (DALC) `RE::Color`s (8-bit RGBA), **and** `lightingTemplateInheritanceFlags.reset(Inherit::kAmbientColor, Inherit::kDirectionalColor)` so the cell's own values are used instead of the template's. In this repo's setup that took effect **live** (no cell re-enter needed) — contrary to the common "exit and re-enter" lore for cell-lighting edits. Save originals to restore. Real ambient (unlike a screen filter) brightens shadows across the whole cell while keeping low-albedo things (black armor) dark. See `src/AmbientBoost.cpp`.
