# Alchemy Spike — Findings

> Code spike, 2026-05-23. Resolves the single riskiest unknown of the
> 3D-physical-alchemy idea (`3D_PHYSICAL_ALCHEMY_FEASIBILITY.md` §1, §9):
> **can an SKSE plugin (CommonLibSSE-NG) produce a vanilla-correct
> `RE::AlchemyItem` from N ingredients, register it so it persists in a save,
> and hand it to the player?**
>
> Scope was deliberately limited to the brew → register → give → log loop.
> Grabbing, fire-magic heat detection, and feedback were NOT built (already
> assessed green in the feasibility doc).

---

## TL;DR / Go-No-Go

- **Mechanically GREEN.** Creating an `AlchemyItem` via `IFormFactory`, filling
  its `effects`, getting the engine to price it, registering it in
  `BGSCreatedObjectManager::potions`, and adding it to the player **all compile
  and link cleanly** (clang-cl cross-compile, `TemplatePlugin.dll` produced).
  The whole loop is callable from a plugin.
- **"Vanilla-correct NUMBERS" = NOT YET PROVEN — must be tested in-game.** The
  spike reconstructs the alchemy math (route b) because no clean vanilla
  brew-function is exposed (see below). The *gold value* is computed by the real
  vanilla function, so value should match. The *magnitude/duration* numbers are
  our reconstruction and are the part most likely to diverge from the vanilla
  `AlchemyMenu` preview. They MUST be eyeballed in-game (procedure below).
- **Persistence = HYPOTHESIS, not confirmed.** We register the potion in the
  `potions` hashmap exactly as the data layout suggests vanilla does, but
  whether the save codec then serializes the dynamic form + entry across a
  save/reload could only be confirmed in-game. This is the **biggest residual
  risk** and the thing the in-game test must check hardest.

**Verdict on the feature's "diegetic but vanilla-correct" pitch:**
**Conditional GO.** The hard structural question ("can a plugin mint and bank a
potion at all?") is answered yes. Two things still gate the pitch and can only
be settled by running the game: (1) do our reconstructed magnitude/duration
numbers equal the vanilla menu's, and (2) does the potion survive a save/reload.
If (1) fails we can fall back to calling more vanilla helpers or, last resort,
ship a self-consistent-but-non-vanilla number set. If (2) fails, dynamic potions
aren't save-safe and the feature must either avoid persistence (consume
immediately) or hook deeper into the menu's creation path.

---

## 1. Which route, and why

**Route taken: (b) reconstruct the math ourselves, leaning on real vanilla
functions wherever CommonLibSSE-NG exposes them.**

### Why not route (a) — call the vanilla brew function directly

There is **no** clean "give me N `IngredientItem`, return an `AlchemyItem`"
function exposed by CommonLibSSE-NG. Confirmed by:

- Grepping the vendored headers/sources: no `CreatePotion`/`MakePotion`/`Brew`/
  `MixIngredient`/`Combine` symbol anywhere.
- `RE/Offsets.h` has a `MagicItem` block (`CalculateCost`, `GetCostliestEffectItem`)
  but **nothing alchemy/potion-creation related**. `BGSCreatedObjectManager`
  exposes only `AddArmorEnchantment`/`AddWeaponEnchantment` — **no `AddPotion`**.
- The real brew logic lives inside the `AlchemyMenu` object
  (`RE::CraftingSubMenus::CraftingSubMenus::AlchemyMenu`): it parses selected
  ingredients into shared effects (`UsableEffectMap`), applies skill/perk
  multipliers, fills `resultPotion`, and banks it. This chain is menu-private
  and is **not** surfaced as a reusable API.

I searched the web/community sources for a trustworthy, **SE+AE-version-paired**
address-library offset for the menu's internal `MakePotion`/result-building
routine and **could not find one I would trust**. Per the task constraint, I did
**not** fabricate an offset. If route (a) is ever wanted, the work item is: open
the `AlchemyMenu::Accept` / result-build path in a disassembler against a known
Address Library DB, identify the function that writes `resultPotion`, and pin
both the SE (1.5.97) and AE (1.6.x) IDs — only then is it safe to call via
`RELOCATION_ID`.

### What route (b) actually uses (real vanilla code where possible)

| Concern | How the spike does it | Vanilla-ness |
|---|---|---|
| Shared-effect detection | Our own intersection of `IngredientItem::effects[].baseEffect` | Faithful: this IS what `UsableEffectMap` represents (effect present on ≥2 ingredients). |
| Skill → strength multiplier | `RE::MagicFormulas::GetWortcraftEffectStrength(alchemySkill)` | **Real vanilla function** (NG wraps it; reads `fWortAlchMult`/`fWortStrMult` GMSTs live). |
| Per-effect magnitude/duration | Our reconstruction: take the stronger contributing ingredient entry, scale by the multiplier, respect `kNoMagnitude`/`kNoDuration` flags | **Reconstruction — the riskiest numeric piece.** |
| Gold value | `MagicItem::CalculateTotalGoldValue(player)` | **Real vanilla function** — it is literally `CalculateCost` at `RELOCATION_ID(11213, 11321)` (see `CommonLibSSE-NG/src/RE/M/MagicItem.cpp`). So value comes from the engine, not our math. |
| Poison vs potion flag | "any contributing effect detrimental" heuristic → set `AlchemyFlag::kPoison` | Approximation; vanilla keys off the dominant/costliest effect. Refine later. |

**Known gaps vs vanilla in the current reconstruction (call these out for the
in-game eyeball test):**
- Perk entry points are **not yet applied**. Vanilla runs
  `BGSEntryPoint::kModPotionsCreated` (id 89, `RE/B/BGSEntryPoint.h:102`),
  `kModAlchemyEffectiveness` (66), `kPurifyAlchemyIngredients` (73), and the
  Purity perk. A character with alchemy perks WILL see different numbers than
  the spike until these are wired in. For an apples-to-apples first test, use a
  character with **no alchemy perks**.
- The "stronger contributing entry" choice and the exact rounding of duration
  scaling are best-guesses; vanilla's precise selection/rounding must be
  verified.
- `GetWortcraftEffectStrength` returns a skill-derived multiplier; the spike
  applies it to magnitude (or duration for no-magnitude effects). Whether
  vanilla applies it identically to BOTH or only one per effect is the key
  thing the in-game numbers will reveal.

---

## 2. Potion registration / persistence — how it works (best understanding)

**What vanilla does (inferred from headers):** when the menu brews a potion it
creates a dynamic `AlchemyItem` and banks it in
`BGSCreatedObjectManager::potions`, a `BSTHashMap<FormID, CreatedMagicItemData>`
where `CreatedMagicItemData = { MagicItem* magicItem; volatile uint32 refCount; }`.
The created-object manager is what serializes dynamic enchantments/potions into
the save (the same manager that owns player-made enchantments via the exposed
`AddArmorEnchantment`/`AddWeaponEnchantment`).

**What the spike does:** because there's no `AddPotion`, it inserts directly:
- lock `mgr->lock` with `BSSpinLockGuard`,
- `mgr->potions.insert({ potion->GetFormID(), { potion, /*refCount*/1, 0 } })`.

This mirrors the data layout vanilla produces. **Risk / unknown I cannot settle
from headers:** whether direct hashmap insertion is *sufficient* for the save
codec to (a) serialize the dynamically-created `AlchemyItem` form and (b)
re-link the inventory reference to it on reload. The vanilla `AddArmor/Weapon
Enchantment` functions may do extra bookkeeping (refcount semantics, dirty
flags, form-flag setup like marking the form as created/dynamic) that a raw
insert skips. **This is the #1 thing the in-game save/reload test must verify.**

If the raw insert proves insufficient, escalation options (in order of
preference):
1. Find & call the real `AddPotion` analogue via a verified `RELOCATION_ID`
   (same hunt as route (a); the function almost certainly sits next to
   `AddArmorEnchantment` `RELOCATION_ID(35264, 36166)` / `AddWeaponEnchantment`
   `(35263, 36165)` in the binary — a strong lead for offset discovery).
2. Mark the form with the correct created/dynamic form flags before insert.
3. Avoid persistence entirely for the MVP (brew → consume immediately), sidestep
   the whole question, and only solve it if "keep the potion" is a requirement.

---

## 3. Cross-compile build result

- Toolchain: `clang-cl` + `lld-link` + xwin (Linux→Windows x64), preset
  `build-release-clang-cl-linux`, `VCPKG_ROOT=/home/lorkhan/vcpkg`.
- `cmake --preset build-release-clang-cl-linux` → configures clean.
  (Note: a fresh configure required wiping `build/release-clang-cl-linux/`; an
  in-place reconfigure over a half-state cache hit a spurious `libcmt.lib not
  found` compiler-probe error. Clean wipe + reconfigure works every time.)
- `cmake --build build/release-clang-cl-linux -j$(nproc)` →
  **compiles `src/alchemy_spike/AlchemySpike.cpp` and links
  `TemplatePlugin.dll` (1.09 MB) with zero errors/warnings** (only the usual
  harmless clang-cl "argument unused" / "-fdelayed-template-parsing deprecated"
  notices, shared by every TU in this repo).

**"Verified" here = clean cross-compile only.** Numeric correctness and
persistence are NOT verified — they require running Skyrim.

---

## 4. Exact in-game test procedure (the part that actually proves the pitch)

Default trigger: **press F11** (DX scancode `0x57`) in-game. The spike brews from
two hardcoded vanilla ingredients and writes everything to the log.

Ingredients chosen: **Blue Mountain Flower `0x00077E1C` + Wheat `0x00034D2C`**.
In vanilla these share exactly the **Restore Health** effect, so the brew should
yield a single-effect Restore Health potion — easy to compare.

Log file (Proton prefix):
```
/home/lorkhan/.local/share/Steam/steamapps/compatdata/489830/pfx/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/TemplatePlugin.log
```

### Test A — numeric correctness vs the vanilla menu
1. Start a save with a character that has **alchemy skill at a known value and
   NO alchemy perks** (perks aren't applied by the spike yet — see §1 gaps).
   `player.setav alchemy 15` from console gives a clean baseline.
2. Open the **vanilla AlchemyMenu** at any alchemy lab, select Blue Mountain
   Flower + Wheat, and **write down the previewed potion**: name, the Restore
   Health magnitude, duration (if any), and gold value.
3. Close the menu. Press **F11**.
4. Open `TemplatePlugin.log`, find the `==== ALCHEMY SPIKE RESULT ====` block.
5. **Compare field-by-field:**
   - `mag` (magnitude) vs the menu's Restore Health points — **must match.**
   - `dur`/`area` vs the menu — must match (Restore Health is instant, so dur 0).
   - `GoldValue` vs the menu's gold value — should match (engine-computed).
   - effect's base MGEF FormID = Restore Health (`0x0003EB24`-family) — sanity.
6. Repeat at a couple of different `alchemy` AV values (e.g. 25, 50, 75) to check
   the `GetWortcraftEffectStrength` scaling tracks the menu across the curve.

   **Pass = spike magnitude/duration equal the menu's at every skill level
   tested. Any mismatch tells us exactly which factor (multiplier application,
   rounding, base-entry selection) is off.**

### Test B — persistence across save/reload (the high-risk one)
1. With the F11-brewed potion in inventory, confirm it shows in the Apothecary's
   satchel / inventory with a sane name ("SPIKE Potion of Restore Health") and
   that drinking it actually restores health (`player.additem` analogue works).
2. **Hard-save** (not just autosave). Note the potion's FormID from the log.
3. Quit to main menu, **reload** the save.
4. Check inventory: is the potion **still there**, with the same name/effects,
   and still drinkable? Also re-open the log after reload.
   - **Pass:** potion persists intact and usable → registration is sufficient,
     persistence GREEN.
   - **Fail modes to watch for:** potion vanishes; potion becomes a blank/
     "<no name>" item; CTD on load; effects reset to zero. Any of these means
     the raw `potions`-insert is insufficient and we escalate per §2.
5. Bonus: brew several potions, save/reload, confirm refcounts/no leaks (watch
   for CTD or duplicated/missing entries).

### Test C — poison branch (optional)
Swap the hardcoded FormIDs to two ingredients sharing a detrimental effect
(e.g. two that share Damage Health) and confirm the spike sets `IsPoison()` and
the menu agrees it's a poison.

---

## 5. Files (this spike)

- `src/alchemy_spike/AlchemySpike.h` — public `AlchemySpike::Init()`.
- `src/alchemy_spike/AlchemySpike.cpp` — the brew→register→give→log loop +
  F11 input sink. Heavily commented; every vanilla-divergence risk is flagged
  inline.
- `cmake/sourcelist.cmake`, `cmake/headerlist.cmake` — registered the new TU/
  header (CMake doesn't glob).
- `src/plugin.cpp` — one `AlchemySpike::Init()` call on `kDataLoaded`, fenced in
  `// >>> alchemy-spike` / `// <<< alchemy-spike` comments for trivial merge/revert.
