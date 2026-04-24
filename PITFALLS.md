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
