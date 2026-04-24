# NPC Generation from Template (SKSE C++)

## Overview
This guide covers how to dynamically create a new NPC at runtime by copying an existing template (like `Player (0x7)`) and spawning them into the world correctly.

## Core Logic: The "Stable Spawning" Pattern

To spawn a functional NPC that doesn't cause CTDs or save-game corruption, follow this multi-step process:

### 1. Identify the Template
Instead of hardcoding every attribute, find a base NPC (e.g., `Player` or `EncCitizen01`) to copy from.
```cpp
auto* templateNPC = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007); // Player
auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
```

### 2. Create and Copy the Base
Create a new dynamic `TESNPC` instance and copy the template's data.
```cpp
auto* newNpcBase = factory->Create()->As<RE::TESNPC>();
newNpcBase->Copy(templateNPC);
newNpcBase->fullName = "Generated Citizen";
```

### 3. Spawning with `PlaceObjectAtMe`
Use the internal engine API `PlaceObjectAtMe` to handle 3D initialization and cell attachment. Do **not** use `CreateReferenceAtLocation` for dynamic NPCs if you want maximum stability.
```cpp
auto spawned = caster->PlaceObjectAtMe(newNpcBase, false);
```

### 4. Position & Rotation Fix
Spawned objects often overlap with the caster. Use trigonometry to move the NPC in front of the caster.
```cpp
float angleZ = caster->data.angle.z; 
RE::NiPoint3 forward(std::sin(angleZ), std::cos(angleZ), 0.0f);

RE::NiPoint3 newPos = caster->GetPosition() + (forward * 150.0f);
spawned->SetPosition(newPos);
spawned->SetAngle(caster->data.angle);
```

## Best Practices
- **Null Checks**: Always verify that `factory`, `template`, and `spawned` are not null.
- **Form Persistence**: Remember that forms in the `0xFF` range (dynamic) may shift IDs after a save/load cycle.
- **AI Initialization**: The NPC will start with the template's AI packages. Use `spawned->As<RE::Actor>()->EvaluatePackage()` to refresh their behavior.
