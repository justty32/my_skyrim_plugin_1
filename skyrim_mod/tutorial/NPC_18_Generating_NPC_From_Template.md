# NPC Generation from Template (SKSE C++)

## Overview
This guide covers how to dynamically create a new NPC at runtime by copying an existing template (like `Player (0x7)`) and spawning them into the world correctly.

## Core Concept: `TESNPC` vs `Actor`
- **`RE::TESNPC`** — the static blueprint: face, default stats, race, skills.
- **`RE::Actor`** — the live instance in the world, created from that blueprint.

Two routes, differing only in whether you reuse an existing `TESNPC` or mint a new one:

| Route | How | Use when |
|---|---|---|
| A. Reuse an existing base | `LookupByID<RE::TESNPC>()`, then spawn | Summoning a guard/wolf — a straight copy of an existing actor |
| B. Clone into a new dynamic base | `IFormFactory` creates a new `TESNPC`, `Copy()` the template, rename/retune, then spawn | You need a different name or stats without touching the original NPC |

## Route A: Spawn from an existing base

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

void SpawnNPCAtPlayer() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    // Whiterun guard
    auto npcBase = RE::TESForm::LookupByID<RE::TESNPC>(0x00012E46);
    if (!npcBase) return;

    auto spawnedRef = player->PlaceObjectAtMe(npcBase, false);
    if (!spawnedRef) return;

    auto newActor = spawnedRef->As<RE::Actor>();
    if (!newActor) return;

    // There is no C++ SetPlayerTeammate(); set the BOOL_BITS flag directly.
    newActor->GetActorRuntimeData().boolBits.set(RE::Actor::BOOL_BITS::kPlayerTeammate);
    newActor->EvaluatePackage();

    RE::DebugNotification(fmt::format("Summoned: {}", newActor->GetName()).c_str());
}
```

## Route B: The "Stable Spawning" Pattern

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
Use the internal engine API `PlaceObjectAtMe` to handle 3D initialization and cell attachment. Do **not** use `CreateReferenceAtLocation` for dynamic NPCs if you want maximum stability — it makes you assemble cell, worldspace and linked-room handles yourself (10 parameters, `include/RE/T/TESDataHandler.h:77`).
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

## Key API Reference (C++ / CommonLibSSE-NG)
- **`RE::TESNPC`** — NPC blueprint record. `include/RE/T/TESNPC.h`
- **`PlaceObjectAtMe()`** — the core spawn function. `include/RE/T/TESObjectREFR.h`
- **`TESNPC::Copy()`** — copy data from another form. `include/RE/T/TESNPC.h`
- **`IFormFactory::GetConcreteFormFactoryByType<T>()`** — factory for a form type. `include/RE/I/IFormFactory.h`
- **`Actor::BOOL_BITS::kPlayerTeammate`** — teammate flag; C++ exposes only the `IsPlayerTeammate()` getter, no setter. `include/RE/A/Actor.h`
- **`Actor::EvaluatePackage()`** — re-evaluate AI packages. `include/RE/A/Actor.h`
- **`RE::DebugNotification()`** — corner notification. `include/RE/M/Misc.h`

## Best Practices
- **Null Checks**: Always verify that `factory`, `template`, and `spawned` are not null.
- **Form Persistence**: Remember that forms in the `0xFF` range (dynamic) may shift IDs after a save/load cycle.
- **AI Initialization**: The NPC will start with the template's AI packages. Use `spawned->As<RE::Actor>()->EvaluatePackage()` to refresh their behavior.
- **Level Scaling**: Dynamically spawned NPCs inherit the template's level-scaling rules automatically.
- **Handles**: Track spawned refs with `RE::ObjectRefHandle`, never raw pointers across frames.
- **Cleanup**: Call `SetDelete(true)` on spawned NPCs you no longer need, or the save file accumulates dead guards.

## Don't Confuse Papyrus with C++

`PlaceAtMe` and `SetPlayerTeammate` are real functions — but they belong to **Papyrus**, not to the C++ interface. Spotting a four-argument `PlaceAtMe(base, 1, false, false)` in C++ code means someone pasted a Papyrus signature.

| Interface | Spawn | Set teammate | Evidence |
|---|---|---|---|
| Papyrus | `ObjectReference Function PlaceAtMe(Form akFormToPlace, int aiCount = 1, bool abForcePersist = false, bool abInitiallyDisabled = false) native` | `Function SetPlayerTeammate(bool abTeammate = true, bool abCanDoFavor = true) native` | `ObjectReference.psc:104`, `Actor.psc:175` |
| C++ (CommonLibSSE-NG) | `NiPointer<TESObjectREFR> PlaceObjectAtMe(TESBoundObject* a_baseToPlace, bool a_forcePersist) const;` | no setter — `IsPlayerTeammate()` plus the `BOOL_BITS::kPlayerTeammate` flag | `include/RE/T/TESObjectREFR.h:454`, `include/RE/A/Actor.h:616` / `:188` |

Searching the whole CommonLibSSE-NG header set for `PlaceAtMe` yields exactly one hit — the condition-function id `kPlaceAtMe = 37` in `include/RE/T/TESCondition.h:75`. No member function by that name exists.
