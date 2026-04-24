# Terrain and Object Manipulation (SKSE C++)

## Overview
This guide focuses on two methods for world modification: shifting existing object heights and spawning new static resources (Trees/Rocks) at runtime.

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
}
```

## Key Engine API: `PlaceObjectAtMe`
Always use `PlaceObjectAtMe` for spawning world objects. This method ensures:
- The reference is correctly linked to the parent cell.
- The 3D model (NIF) is loaded and attached to the scene graph.
- Physics and collision data are initialized.

## Best Practices
- **Z-Axis Ground Alignment**: After moving an object, it might be floating. Consider raycasting to find the exact ground height.
- **Reference Handles**: Use `RE::ObjectRefHandle` for tracking spawned objects safely across frames.
- **Persistence**: Dynamically spawned objects in the `0xFF` range will be cleaned up by the engine if they aren't properly flagged as persistent or if the cell is reloaded without being in a save file.
