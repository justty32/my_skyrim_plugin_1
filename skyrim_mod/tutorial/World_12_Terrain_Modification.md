# Terrain and Object Manipulation (SKSE C++)

## Overview
This guide focuses on world modification: shifting existing object heights, spawning new static resources (Trees/Rocks) at runtime, and the hard limits on actually reshaping terrain from a plugin.

## 1. Modifying Existing Object Z-Height
To raise or lower an object under the crosshair, you must find the reference and update its 3D position.

```cpp
auto* target = GetCrosshairTarget();
if (target) {
    auto pos = target->GetPosition();
    pos.z += 50.0f; // Raise by 50 units
    target->SetPosition(pos);
}
```
*Note: This modification affects the specific reference in the current cell. For persistent changes, the cell's modified state will be stored in the save file.*

### Getting the crosshair target
CommonLibSSE-NG has **no** `SKSE::GetCrosshairRef()`. Subscribe to `SKSE::CrosshairRefEvent` (`include/SKSE/Events.h:30`) via `SKSE::GetCrosshairRefEventSource()` (`include/SKSE/API.h:36`) and cache the `ObjectRefHandle` yourself.

## 2. Spawning Static World Resources (Trees, Rocks)
Spawning static objects like `TreeFloraJuniper01` or `RockPileM01` requires robust resource lookup and correct engine-level initialization.

### Robust Lookup by EditorID
Avoid FormIDs (e.g., `0x38432`) as they may differ in some setups. EditorID lookup is extremely stable.

```cpp
auto* treeBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("TreeFloraJuniper01");
if (!treeBase) {
    // Fallback logic
    treeBase = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("RockPileM01");
}
```

### Positioning Logic (The Offset Pattern)
To ensure the spawned object doesn't collide with the player, calculate a forward offset vector.

```cpp
float angleZ = player->data.angle.z; 
RE::NiPoint3 forward(std::sin(angleZ), std::cos(angleZ), 0.0f);

// Spawn 200 units in front of player
RE::NiPoint3 spawnPos = player->GetPosition() + (forward * 200.0f);

auto spawned = player->PlaceObjectAtMe(treeBase, false);
if (spawned) {
    spawned->SetPosition(spawnPos);
    spawned->SetAngle(player->data.angle);

    // Random yaw looks more natural. SetAngle only takes an NiPoint3 —
    // there is no three-float overload.
    float randomRot = static_cast<float>(rand() % 360);
    spawned->SetAngle(RE::NiPoint3(0.0f, 0.0f, randomRot));
}
```

## Key Engine API: `PlaceObjectAtMe`
Always use `PlaceObjectAtMe` for spawning world objects. This method ensures:
- The reference is correctly linked to the parent cell.
- The 3D model (NIF) is loaded and attached to the scene graph.
- Physics and collision data are initialized.

Signature: `NiPointer<TESObjectREFR> PlaceObjectAtMe(TESBoundObject* a_baseToPlace, bool a_forcePersist) const;` (`include/RE/T/TESObjectREFR.h:454`). The Papyrus counterpart is `PlaceAtMe` (`ObjectReference.psc:104`) — different interface, do not mix the two signatures in C++.

## 3. On "Terrain Sculpting" — what a plugin cannot do

**Warning**: Skyrim's terrain (heightmap) comes from pre-generated `.btr` / `.bnd` data files. That is a different system from static objects.

### Why in-plugin terrain editing is impractical
- **Static by design**: the engine reads the heightmap into physical memory when a cell loads; live editing needs very deep kernel-level hooks.
- **Navmesh breakage**: the navmesh does not follow a changed heightmap, so NPCs end up floating above or walking through the ground.

### Practical alternatives
1. **Spawn a "terrain patch"** — to raise ground, don't touch the heightmap; place a large landscape static instead. This is how most wall-of-stone style mods work.
2. **Use decals** — for purely visual ground changes (scorch marks, ice), use `RE::BGSDecalManager` (`include/RE/B/BGSDecalManager.h`).

## Key API Reference
- **`TESObjectREFR::PlaceObjectAtMe`** — spawn a reference. `include/RE/T/TESObjectREFR.h:454`
- **`TESForm::LookupByEditorID<T>`** — form lookup by EditorID. `include/RE/T/TESForm.h:247`
- **`TESObjectREFR::SetPosition` / `SetAngle`** — both take `NiPoint3`. `include/RE/T/TESObjectREFR.h:464` / `:458`
- **`TESObjectTREE`** — tree class with wind animation. `include/RE/T/TESObjectTREE.h`
- **`RE::NiPoint3`** — position math. `include/RE/N/NiPoint3.h`
- **`RE::BGSDecalManager`** — ground decals. `include/RE/B/BGSDecalManager.h`

## Best Practices
- **Z-Axis Ground Alignment**: After moving an object, it might be floating. Consider raycasting to find the exact ground height.
- **Reference Handles**: Use `RE::ObjectRefHandle` for tracking spawned objects safely across frames.
- **Persistence**: Dynamically spawned objects in the `0xFF` range will be cleaned up by the engine if they aren't properly flagged as persistent or if the cell is reloaded without being in a save file.

## Summary
"Raising mountains" and "planting trees" in Skyrim both come down to calling `PlaceObjectAtMe` with the right static at the right position. Unless you are building an engine-level editor, do not try to edit the heightmap data directly.
