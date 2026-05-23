# 3D 物理化煉金系統 — SKSE 可行性分析

> 探索性可行性分析，2026-05-23。由 research agent 產出，所有引擎面論斷均對照本 repo vendored 的
> `CommonLibSSE-NG/include/RE/` 標頭查證。**僅分析，未動 plugin 程式碼。**

分析對象：把 Skyrim 選單式煉金（AlchemyMenu）換成「用滑鼠抓取材料 → 丟進鍋子 → 用火焰魔法加熱 → 釀出藥水」的具身（diegetic）物理互動。

---

## 1. 原生煉金內部 — 能不能直接呼叫「材料組合 → 藥水」邏輯？

**已查證的 header（都存在、佈局完整）**
- `RE/A/AlchemyItem.h` — 繼承 `MagicItem`，藥水成品就是 `AlchemyItem`，效果存在繼承來的 `effects`（`BSTArray<Effect*>`）。
- `RE/I/IngredientItem.h` — 材料；`GetMaxEffectCount()` 回 `4`，有 `LearnEffect/LearnAllEffects`（已知效果旗標 `gamedata.knownEffectFlags`），`GetAssociatedSkill()` 回 `kAlchemy`（header 標 `kConfidence` 是逆向誤標）。
- `RE/M/MagicItem.h` — 兩者共同基底：`CalculateTotalGoldValue`、`GetCostliestEffectItem`、`GetLongestDuration`、`GetLargestArea`、`Traverse(MagicItemTraversalFunctor&)`、`CollectData()`。
- `RE/A/AlchemyMenu.h`（實為 `RE::CraftingSubMenus::CraftingSubMenus::AlchemyMenu`）— 關鍵成員：`PotionCreationData potionCreationData`（含 `UsableEffectMap` 共享效果配對表、`MenuIngredientEntry`）、`AlchemyItem* resultPotion`、`AlchemyItem* unknownPotion`、`InventoryEntryData* resultPotionEntry`、`bool playerHasPurityPerk`。
- `RE/M/MagicFormulas.h` — 暴露 `MagicFormulas::GetWortcraftEffectStrength(float a_alchemySkill)`（釀造強度公式核心，技能 → 倍率）。
- `RE/B/BGSCreatedObjectManager.h` — singleton `GetSingleton()`；成員 `BSTHashMap<...> potions / poisons`、`weaponEnchantments / armorEnchantments`，有 `AddArmorEnchantment / AddWeaponEnchantment`（**注意：暴露了附魔加入函式，但沒有 `AddPotion`**）。動態造的藥水需登記在此才能正確存檔與引用計數。
- `RE/I/ItemCrafted.h` — `ItemCrafted::Event{ TESForm* item; ... }` + `GetEventSource()`，「打造完成」事件來源。

**核心結論 — 沒有現成、乾淨可呼叫的「給我 N 個 IngredientItem，回傳一個 AlchemyItem」函式被 CommonLibSSE-NG 暴露。** 全程搜尋 `CreatePotion/MakePotion/MixIngredient/Brew/Combine` 等命名**找不到**。釀造真正流程綁在 menu 物件上：把選取材料解析成共享效果（`UsableEffectMap`）、套用技能/perk 倍率（`GetWortcraftEffectStrength` + perk entry point `kModPotionsCreated`，見 `RE/B/BGSEntryPoint.h:102`）、產生 `resultPotion` 並登記到 `BGSCreatedObjectManager::potions`。這整條鏈是 menu 私有邏輯，**未被當成可重用 API 暴露**。

兩條路：
- **(R&D spike) 自己造**：用 `IFormFactory::GetConcreteFormFactoryByType<RE::AlchemyItem>()` 造 `AlchemyItem`（本 repo `NpcGenerator.cpp:262-298` 已用 `<EffectSetting>`/`<SpellItem>` 證實此手法），自己比對 `IngredientItem::effects` 算共享效果（公開資料），自套倍率。**風險在「數值絕對 vanilla 正確」**：magnitude/duration/cost、純度 perk、技能倍率、`GetWortcraftEffectStrength` 用法需逆向核對。
- **(較穩) 復用 vanilla 計算函式，用 `RELOCATION_ID` 直呼**：需自己從 Address Library 找出 menu 內部產生 `resultPotion` 的函式偏移（CommonLib 沒包好，**ID 自己查**）。典型逆向工程。

**抑制 vanilla AlchemyMenu — 可行**，兩種社群已驗證做法：
1. 訂閱 `RE::MenuOpenCloseEvent`（`RE/M/MenuOpenCloseEvent.h`，`menuName`+`opening`），偵測 `CraftingMenu`（`RE/C/CraftingMenu.h:19` `MENU_NAME = "Crafting Menu"`）打開且子類是 alchemy 時，`UIMessageQueue::AddMessage(..., kHide, ...)`。會有「閃一下」風險。
2. 更乾淨：hook 煉金台 reference 的 activate（`TESActivateEvent` 或 hook `TESObjectREFR::ActivateRef`），在 vanilla 開選單前攔截、改走自己的 3D 流程。R&D（要找 activate hook 點），但社群多有先例。

**判定**：資料結構齊全可讀（well-supported）；「重用 vanilla 釀造計算」= R&D spike；抑制選單 = 可行但需處理 hook 細節。

---

## 2. 3D 物件抓取 / 放置

**抓取系統暴露非常完整（最大利多）** — `RE/P/PlayerCharacter.h` 直接提供：
- `void StartGrabObject()` / `void EndGrabObject()`（`#ifndef ENABLE_SKYRIM_VR`，SE 可用）
- `NiPointer<TESObjectREFR> GetGrabbedRef()`、`bool IsGrabbing() const`
- `void ActivatePickRef()`、`void DestroyMouseSprings()`、`void UpdateCrosshairs()`
- runtime data：`grabbedObject`（`ObjectRefHandle`）、`grabObjectWeight`、`telekinesisDistance`、`grabDistance`、`grabType`（`GrabbingType { kNone, kNormal, kTelekinesis }`）、`BSTSmallArray<hkRefPtr<bhkMouseSpringAction>, 4> grabSpring`（Havok 滑鼠彈簧——Z 鍵抓取的底層機制）。

**準星 / 射線**
- `RE/C/CrosshairPickData.h` — singleton，成員 `target`、`targetActor`、**`grabPickRef`**、`collisionPoint`（NiPoint3）、`targetCollider`（`bhkRigidBody*`）、`pickCollider`。本 repo 已用（`NpcGenerator.cpp:21-28` `GetCrosshairTarget()`）。⚠️ header 裡 `target` 是單一 `ObjectRefHandle`，但 repo 程式碼用 `crosshair->target[0]`——NG 不同版本佈局有差，整合要核對（小坑）。
- 相機射線：`RE/B/bhkPickData.h`（`hkpWorldRayCastInput/Output` + collectors）配 `RE/T/TES.h:NiAVObject* Pick(bhkPickData&)`，以及 `MagicCaster::FindPickTarget(...)`、`TestProjectilePlacement(...)`。

**放置**：本 repo 已有成熟範例——`TESObjectREFR::PlaceObjectAtMe`（`NpcGenerator.cpp:78,167`）、`TESDataHandler::CreateReferenceAtLocation`（:48，精準座標）、`SetPosition/SetAngle`。`src/util.h` 的 `NifUtil`（clone NiAVObject `RELOCATION_ID(68835,70187)`、attach bone、`Collision::ToggleMeshCollision` 用 `CFilter::Flag::kNoCollision`）可做「材料漂浮到鍋口」或關碰撞做吸入。

**判定**：抓取/放置是本案**最被支援、有大量現成 mod 佐證**的部分。well-supported。唯一細節坑：`CrosshairPickData::target` 跨版本佈局。

---

## 3. 偵測「材料被丟進鍋子」

四個選項，依可靠度排序：
1. **鍋子做成容器，訂閱 `TESContainerChangedEvent`（`RE/T/TESContainerChangedEvent.h`）** — `oldContainer/newContainer/baseObj/itemCount/reference`。最乾淨。缺點：物理掉落 ≠ 引擎「activate 撿取」語意；需鄰近偵測把掉落 refr「吸收」轉成 `AddObjectToContainer`（`RE/T/TESObjectREFR.h:286`）。
2. **鄰近輪詢 `TES::ForEachReferenceInRange(origin, radius, callback)`（`RE/T/TES.h:70`）** — 掃描鍋口半徑內的 `IngredientItem` refr，命中即吸收。簡單、可靠、無需 Havok 接觸回呼。**推薦作為 MVP 偵測法。**
3. **`TESGrabReleaseEvent`（`RE/T/TESGrabReleaseEvent.h`，`ref`+`grabbed`）** — 監聽玩家「放開」抓取物，放開瞬間若 `GetGrabbedRef()` 在鍋口範圍內即視為投入。配合 §2 抓取系統最自然。
4. **Havok 接觸回呼** — `bhk*`/`hkp*` 系統存在（144 個 header），但 CommonLibSSE-NG **沒暴露現成好用的「兩 refr 接觸事件 sink」**。自掛 Havok contact listener 屬深水區，**不建議**作為主力。

**判定**：鄰近輪詢可行且簡單；容器事件可行但語意需橋接；Havok 接觸回呼 = blocker 級難度（不必要）。

---

## 4. 火焰魔法作為熱源

**魔法系統 header 齊全**
- `RE/M/MagicCaster.h` + `RE/A/ActorMagicCaster.h` — `State { kCharging, kCasting, ... }`、`currentSpell`、`GetCastingSource()`；`Actor::GetMagicCaster(MagicSystem::CastingSource)`（`RE/A/Actor.h:306`）與 runtime data `magicCasters[]`/`selectedSpells[]`（`Actor.h:679-680`）讓你讀玩家此刻左右手在充能/釋放什麼。
- 火焰判定：`RE/E/EffectSetting.h` 的 `data.resistVariable`（火系 = `ActorValue::kResistFire`，`RE/A/ActorValues.h:49 kResistFire = 41`）、`data.associatedSkill`（`kDestruction`）、`GetArchetype()`。遍歷法術 `effects[].baseEffect` 判斷是否火系。
- 命中事件：`RE/T/TESMagicEffectApplyEvent.h`（`target/caster/magicEffect`）。**坑**：fire-and-forget 投射物的效果通常只 apply 在 `Actor`/`MagicTarget` 上；鍋子若是普通 activator/static **多半不會**收到。`RE/P/Projectile.h`/`RE/M/MissileProjectile.h`/`RE/E/Explosion.h` 存在，可走「投射物/爆炸命中 refr」路徑但要 hook，較重。

**務實代理（強烈建議）**：不追求「火球精準命中鍋子碰撞點」，改用——玩家**裝備了**火系法術（`GetEquippedObject` 或 `selectedSpells[]`，判 `EffectSetting.resistVariable == kResistFire`）**且**正在 charging/casting（`MagicCaster::state`）**且**準星目標/鍋子在前方近距（`CrosshairPickData::target` 或 `ForEachReferenceInRange`）。三者成立即視為「正在加熱」，累積熱量計時。全靠已暴露 API，無需 hook 投射物。

**判定**：「裝備火系 + 正在施法 + 鍋在範圍」= 可行（well-supported）；「火球投射物精準命中鍋」= R&D spike。

---

## 5. 生成成品 + 回饋（全可行，本 repo 多已具備）

- **給藥水**：`TESObjectREFR::AddObjectToContainer(...)`（`RE/T/TESObjectREFR.h:286`，cookbook R6 已用）。但動態造的 `AlchemyItem` 需登記 `BGSCreatedObjectManager::potions`（見 §1），否則存檔/引用可能異常——**待驗證點**。
- **蒸汽/粒子**：`TESObjectREFR::InstantiateHitArt(BGSArtObject*, dur, ...)`（:438）掛 art object，或 `ApplyEffectShader(TESEffectShader*, ...)`（:369）；也可 `RE/M/ModelReferenceEffect.h`/`RE/B/BSTempEffect.h`。**好看的蒸汽需現成 art object/effect shader 素材**——可借 vanilla 既有（烹飪鍋蒸氣、附魔台特效）的 `BGSArtObject`，免做新資產；客製視覺則需外部 NIF/特效素材。
- **音效**：`RE/B/BSAudioManager.h` `Play(FormID)` / `Play(BSISoundDescriptor*)` / `BuildSoundDataFromEditorID`。用 vanilla 沸騰/施法音效即可。
- **計時**：`SKSE::GetTaskInterface()` + 自管計時器，或訂閱 update。本 repo 一切遊戲狀態操作都包進 `GetTaskInterface()->AddTask`（cookbook R9 鐵則 1）。

**判定**：邏輯全可行；視覺/音效沿用 vanilla 資產 = 可行，客製資產 = 需外部素材。

---

## 6. 輸入 / 游標

**header**：`RE/B/BSInputDeviceManager.h`、`RE/C/ControlMap.h`、`RE/I/InputEvent.h`（`AsButtonEvent`/`AsMouseMoveEvent`）、`RE/B/ButtonEvent.h`、`RE/P/PlayerControls.h`、`RE/M/MouseMoveEvent.h`、`RE/C/CursorMenu.h`。`src/util.h` 的 `KeyUtil` 已處理鍵碼。

**關鍵限制 — 自由 3D 滑鼠游標不切實際**：`MouseMoveEvent` 只給 `mouseInputX/Y` 的**相對位移（delta）**，不是螢幕絕對座標；遊玩中游標被隱藏、delta 拿去轉相機。要做「螢幕自由游標 → 反投影世界射線抓物」必須自己接管滑鼠（攔截 input 阻止轉相機）、自繪游標、做螢幕→世界反投影——一整套 R&D，且與第一/三人稱操作衝突。

**務實互動模型（強烈建議）**：放棄自由游標，改用**準星看向 + 啟動鍵**——用準星（`CrosshairPickData`）決定看著哪個材料/鍋子，用既有抓取鍵配 `StartGrabObject`/`GetGrabbedRef`/`TESGrabReleaseEvent` 抓放，或註冊自訂 hotkey（`InputEvent` sink + `KeyUtil`）。與引擎原生操作一致，§2 證明抓取 API 已完整暴露。

**判定**：自由游標 = R&D spike 兼體驗風險（建議捨棄）；準星看向 + 啟動/抓取鍵 = 可行。

---

## 7. 分層總結

**綠燈（已支援 / 有現成 mod 佐證）**
- 抓取與放置：`StartGrabObject/EndGrabObject/GetGrabbedRef/IsGrabbing`、Havok mouse spring、`PlaceObjectAtMe`、`CreateReferenceAtLocation`。
- 偵測入鍋：`TES::ForEachReferenceInRange` 鄰近輪詢 + `TESGrabReleaseEvent`。
- 火焰加熱（代理法）：讀 `GetMagicCaster` 狀態 + 裝備火系判斷 + 鍋在範圍。
- 回饋：`AddObjectToContainer`、`InstantiateHitArt`/`ApplyEffectShader`、`BSAudioManager::Play`（沿用 vanilla 資產）。
- 動態造 form：本 repo `IFormFactory` 模式。
- 互動模型：準星看向 + 啟動鍵。

**黃燈（R&D spike，需逆向 / 找偏移 / 自寫公式）**
- **數值 vanilla 正確的藥水產出**：找 menu 內部產生 `resultPotion` 的函式偏移直呼，或自寫共享效果解析 + 倍率（可借 `GetWortcraftEffectStrength`）。**本案最大不確定性。**
- 動態 `AlchemyItem` 正確登記 `BGSCreatedObjectManager::potions` 與存檔持久化。
- 抑制 vanilla AlchemyMenu（menu hide 或 activate hook）。
- 「火球投射物精準命中鍋」（若堅持，需 hook projectile/explosion）。
- `CrosshairPickData::target` 跨版本佈局核對。

**紅燈（阻斷 / 不值得）**
- 自由 3D 滑鼠游標反投影抓取（與相機操作衝突，建議捨棄）。
- 自掛 Havok 兩-refr 接觸事件作為主力（鄰近輪詢可完全取代）。
- 客製蒸汽/釀造視覺資產（需外部 NIF/特效；先用 vanilla 資產）。

---

## 8. 相關前例（web；注意本案是 SE 不是 VR）

- **抓取/物理（直接佐證綠燈）**：[Grab And Throw](https://www.nexusmods.com/skyrimspecialedition/mods/120460)、[Drag and Drop](https://www.nexusmods.com/skyrimspecialedition/mods/178446)（用引擎 Havok mouse spring，正是本案機制）、[Better Grabbing](https://www.nexusmods.com/skyrimspecialedition/mods/134769)、[Better Telekinesis](https://www.nexusmods.com/skyrimspecialedition/mods/42906)。
- **互動框架**：[Object Impact Framework (OIF)](https://www.nexusmods.com/skyrimspecialedition/mods/149484) — 對物件 activate/grab/release/throw/hit 加自訂效果，可參考其 grab/release 事件接法。
- **runtime 生成藥水配方（直接佐證黃燈可達成）**：[Alchemy Reworked](https://www.nexusmods.com/skyrimspecialedition/mods/105386)（附 SKSE plugin，**執行期依材料效果自動生成配方**）、[CACO](https://www.nexusmods.com/skyrimspecialedition/mods/19924)。
- **物理鍋具/容器（多為 Papyrus + 資產）**：[Alchemist's Cauldron](https://www.nexusmods.com/skyrimspecialedition/mods/36679)。
- **觀察**：真正「丟材料進鍋 + 火加熱釀造」具身互動主要出現在 **VR**（[VR Equip Potions And Ingredients](https://www.nexusmods.com/skyrimspecialedition/mods/147691)）；**SE 平面版沒有現成同類**——既說明創新空間，也說明難點在「平面操作下的抓取/瞄準」（§2、§6 已給 SE 的務實解）。

---

## 9. 最小可行驗證（MVP）與最該先解的風險

**最小垂直切片（證明核心互動迴圈，不碰最難的數值正確性）**
1. 一個煉金台（或任何 activator）refr 當「鍋子」（先用既有 vanilla furniture/activator，不做新資產）。
2. 玩家用既有抓取鍵抓起一個 `IngredientItem` 世界 refr → `GetGrabbedRef()`；放開時 `TESGrabReleaseEvent` 觸發。
3. 放開瞬間用 `TES::ForEachReferenceInRange` 確認落點在鍋口半徑內 → 記錄材料、`Disable()` 該 refr（視覺「進鍋」）、`InstantiateHitArt` 加小水花（vanilla art）。
4. 玩家裝備火系並對鍋施法：讀 `GetMagicCaster(kRightHand/kLeftHand)->state == kCasting` 且裝備法術 `EffectSetting.resistVariable == kResistFire` 且鍋在範圍 → 累積熱量計時。
5. 熱量足夠 → `IFormFactory` 造 `AlchemyItem`（**MVP 數值先用簡化/佔位**），`AddObjectToContainer` 給玩家，`BSAudioManager::Play` 播沸騰音、`InstantiateHitArt` 出蒸汽，log 記錄。

把 §2/3/4/5/6 綠燈全串起來，**刻意把 §1 的「vanilla 數值正確」延後**。

**最該先解的單一未知數**：§1 的「**藥水數值是否 vanilla 正確 + 動態 `AlchemyItem` 能否正確登記 `BGSCreatedObjectManager::potions` 並持久化存檔**」。建議第一個 spike 就專做：「給定 2 個寫死材料，產生數值與 vanilla AlchemyMenu 完全一致的藥水並能存檔重載」。若走不通（被迫自寫整套煉金公式且難對齊 vanilla），整個 mod 的「diegetic 但 vanilla-correct」賣點需重新定位（退而求其次：自定義一套自洽但非 vanilla 的數值）。抓取與火焰偵測雖要寫，但 API 齊全、風險低；**數值正確性才是成敗關鍵**。

**工作量評估**：抓取/偵測/火焰/回饋/互動模型靠已暴露 API + 本 repo 既有模式（`NpcGenerator.cpp`、`util.h` 的 `NifUtil`/`MathUtil`），中等且低風險。真正吃時間與風險的是（a）藥水數值對齊 vanilla（逆向或自寫 + `BGSCreatedObjectManager` 持久化），(b) 乾淨抑制 vanilla 選單的 hook。**建議先做數值 spike，再做互動切片，最後打磨視覺/音效（儘量沿用 vanilla 資產）。**
