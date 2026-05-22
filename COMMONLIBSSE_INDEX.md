# CommonLibSSE-NG 索引（繁體中文）

本檔是 `CommonLibSSE-NG/include/` 底下可用內容的速查索引，幫你快速知道「有哪些東西可以用、該去哪個資料夾找」。
不是逐檔清單（光 `RE/` 就有 1631 個 header），而是依系統分類，列出最常用的進入點。

> 找不到某個 class？檔案是依**首字母**分資料夾的，例如 `RE/A/Actor.h`、`RE/T/TESForm.h`。
> 想看大全覽就翻 `RE/Skyrim.h`（彙整了大量 include）。

---

## 0. 四大命名空間 + Flash

| 命名空間 | 路徑 | 用途 | 檔案量 |
|----------|------|------|--------|
| **`RE`** | `include/RE/` | 逆向工程出來的「遊戲類別」——幾乎所有 Skyrim 引擎 class 都在這 | ~1631 |
| **`SKSE`** | `include/SKSE/` | SKSE API 封裝層——寫 plugin 最常用：log、訊息、序列化、trampoline | 17 |
| **`REL`** | `include/REL/` | Address Library / 重定位——跨遊戲版本找函式位址 | 8 |
| **`REX`** | `include/REX/` | 平台底層（W32 / PS4）——Win32 API 包裝 | 28 |
| **`Flash`** | `Flash/` | Scaleform / ActionScript UI 素材 | — |

寫 plugin 的典型分工：**SKSE 拿介面 → REL 找位址做 hook → RE 操作遊戲物件**。

---

## 1. SKSE — API 封裝層（最常用，先看這個）

進入點 header：`SKSE/SKSE.h`、`SKSE/API.h`、`SKSE/Interfaces.h`、`SKSE/Logger.h`、`SKSE/Trampoline.h`、`SKSE/Events.h`。

### 介面（透過 `SKSE::GetXxxInterface()` 取得）
| 函式 | 拿到的介面 | 做什麼 |
|------|-----------|--------|
| `GetMessagingInterface()` | `MessagingInterface` | plugin 間訊息、生命週期事件（kDataLoaded 等） |
| `GetSerializationInterface()` | `SerializationInterface` | 存檔/讀檔時保存自訂資料（co-save） |
| `GetTaskInterface()` | `TaskInterface` | 把工作丟到主執行緒 / 下一個 frame 執行 |
| `GetPapyrusInterface()` | `PapyrusInterface` | 註冊原生函式給 Papyrus 腳本呼叫 |
| `GetScaleformInterface()` | `ScaleformInterface` | 註冊 Scaleform UI 回呼 |
| `GetTrampolineInterface()` | `TrampolineInterface` | 配置 trampoline 記憶體做 hook |
| `GetObjectInterface()` | `ObjectInterface` | 物件/委派管理 |

### SKSE 命名空間自由函式
`GetActionEventSource()`、`GetCameraEventSource()`、`GetCrosshairRefEventSource()`、
`GetModCallbackEventSource()`、`GetNiNodeUpdateEventSource()`、`GetDelayFunctorManager()`、
`GetObjectRegistry()`、`GetPersistentObjectStorage()`、`GetPluginHandle()`、`GetReleaseIndex()`。

### SKSE 內建事件型別（`SKSE/Events.h`）
`ActionEvent`（武器揮擊/施法/弓箭階段）、`CameraEvent`、`CrosshairRefEvent`（準星指向的物件）、
`ModCallbackEvent`（Papyrus ↔ C++ 自訂事件）、`NiNodeUpdateEvent`。

### 其他實用 header
- `SKSE/Logger.h` — spdlog 包裝（本專案 `src/log.h` 已接好）。
- `SKSE/Trampoline.h` — 5/6-byte branch/call hook。
- `SKSE/RegistrationMap*.h` / `RegistrationSet*.h` — 管理 Papyrus 事件訂閱者並隨存檔序列化。
- `SKSE/InputMap.h` — 鍵碼對應。
- `SKSE/Translation.h` — `$key` 多語系字串。

---

## 2. REL — 位址重定位（做 hook / 呼叫遊戲函式時用）

| header | 內容 |
|--------|------|
| `REL/ID.h` | `REL::ID`、`REL::RelocationID`（給 SE/AE 各一組 ID） |
| `REL/Relocation.h` | `REL::Relocation<T>`、`RELOCATION_ID(seID, aeID)` 巨集（本專案 `util.h` 大量使用） |
| `REL/Offset.h` | `REL::Offset`（直接給偏移量） |
| `REL/Module.h` | 偵測遊戲版本（SE / AE / VR / GOG） |
| `REL/Version.h` | 版本號比較 |
| `REL/Pattern.h` | byte pattern 掃描 |

> 用法慣例：`REL::Relocation<func_t> func{ RELOCATION_ID(seID, aeID) };` —— ID 仍需自己從 Address Library 查。

---

## 3. RE — 遊戲類別（依系統分類）

### 3.1 Form 系統（一切資料的根）
所有可被 ESP/ESM 定義的東西都繼承 `TESForm`。檔案多在 `RE/T/`（`TES*`，138 個）與 `RE/B/`（`BGS*`，147 個）。

- **基底**：`TESForm`、`TESBoundObject`、`TESObjectREFR`（場景中的實例參照）、`TESDataHandler`（查找/列舉所有 form）。
- **角色/NPC**：`TESNPC`（基底資料）、`Actor` / `Character` / `PlayerCharacter`（場上實例）、`TESRace`、`TESClass`、`TESFaction`、`TESCombatStyle`。
- **物品**：`TESObjectWEAP`（武器）、`TESObjectARMO`（防具）、`TESObjectARMA`（穿戴模型）、`TESObjectBOOK`、`TESObjectMISC`、`TESSoulGem`、`TESAmmo`、`TESKey`、`AlchemyItem`、`IngredientItem`、`ScrollItem`。
- **魔法**：`SpellItem`、`EnchantmentItem`、`EffectSetting`（魔法效果定義 MGEF）、`TESShout`、`TESWordOfPower`、`MagicItem`。
- **世界**：`TESObjectCELL`、`TESWorldSpace`、`TESObjectSTAT`、`TESObjectACTI`（可互動）、`TESObjectDOOR`、`TESObjectTREE`、`TESFlora`、`TESObjectLIGH`、`TESWaterForm`、`TESWeather`、`TESClimate`、`TESRegion`。
- **邏輯/資料**：`TESQuest`、`TESTopic` / `TESTopicInfo`（對話）、`TESGlobal`（全域變數）、`TESLeveledList` / `TESLevItem` / `TESLevCharacter` / `TESLevSpell`、`BGSKeyword`、`BGSLocation`、`BGSPerk`、`ActorValueInfo`。
- **共用元件**（mixin）：`TESFullName`（顯示名稱）、`TESModel`（nif 模型）、`TESContainer`（內容物）、`TESEnchantableForm`、`TESHealthForm`、`TESValueForm`（價格）、`TESWeightForm`、`BGSBipedObjectForm`（裝備部位）。

### 3.2 角色與 AI
`Actor`、`Character`、`PlayerCharacter`、`ActorState`、`ActorEquipManager`（裝備/卸除）、
`ActorMover`、`ActorMagicCaster`、`AIProcess` / `ProcessLists`（AI 進程，含載入角色清單）、
`TESPackage`（AI 包）、`CombatController` / `CombatManager`、`MovementControllerNPC`。

### 3.3 魔法系統
- 定義：`SpellItem`、`EffectSetting`（MGEF）、`Effect`（spell 內單一效果條目）、`MagicItem`。
- 施法：`ActorMagicCaster`、`MagicCaster`、`MagicTarget`、`TESMagicCasterForm` / `TESMagicTargetForm`。
- 生效中的效果：`ActiveEffect`（基底）+ 57 個 `*Effect` 實作，例如
  `ValueModifierEffect`、`SummonCreatureEffect`、`CommandEffect`、`CloakEffect`、
  `ParalysisEffect`、`InvisibilityEffect`、`SlowTimeEffect`、`TelekinesisEffect`、
  `DetectLifeEffect`、`ReanimateEffect`、`WerewolfEffect`、`VampireLordEffect` …（全在各字母夾，檔名 `*Effect.h`）。

### 3.4 物品欄 / 容器
`TESContainer`、`InventoryEntryData`、`ExtraDataList`（單件物品的額外資料：附魔、充能、命名…）、
`InventoryChanges`、`Inventory3DManager`、`ContainerMenu` / `InventoryMenu`、`BarterMenu`。

### 3.5 UI / 選單（41 個 `*Menu`）
全部繼承 `IMenu`，透過 `UI`（singleton）與 `UIMessageQueue` 開關。常用：
`HUDMenu`、`InventoryMenu`、`MagicMenu`、`MapMenu` / `LocalMapMenu`、`DialogueMenu`、
`JournalMenu`、`MessageBoxMenu`、`ContainerMenu`、`BarterMenu`、`CraftingMenu` / `SmithingMenu` / `AlchemyMenu` / `EnchantConstructMenu`、`FavoritesMenu`、`LevelUpMenu`、`LoadingMenu`、`MainMenu`、`TweenMenu`、`CursorMenu`、`ConsoleNativeUIMenu`。
相關：`UI`、`UIMessage`、`MenuControls`、`MenuOpenCloseEvent`、`BSScaleformManager`、`GFxMovieView`、`GFxValue`（操作 Scaleform）。

### 3.6 事件（hook 遊戲行為的主要途徑）
- **`ScriptEventSourceHolder`**（singleton）—— 一站式取得遊戲事件來源，做 `BSTEventSink<T>` 訂閱。可用事件超過 50 種，例如：
  `TESActivateEvent`、`TESCombatEvent`、`TESContainerChangedEvent`、`TESDeathEvent`、
  `TESEquipEvent`、`TESHitEvent`、`TESLoadGameEvent`、`TESMagicEffectApplyEvent`、
  `TESObjectLoadedEvent`、`TESSpellCastEvent`、`TESCellAttachDetachEvent`、
  `TESQuestStageEvent`、`TESSleepStartEvent` / `TESSleepStopEvent`、`TESWaitStartEvent`/`Stop`、
  `TESTrackedStatsEvent`、`TESGrabReleaseEvent`、`TESFurnitureEvent`、`TESPackageEvent` …
- **訂閱模式**：自訂 class 繼承 `BSTEventSink<TESHitEvent>`、實作 `ProcessEvent(...)`，再 `holder->AddEventSink<TESHitEvent>(this)`。
- **輸入事件**：`InputEvent` / `ButtonEvent` / `BSInputDeviceManager`、`BSInputEventReceiver`。
- **動畫事件**：`BSAnimationGraphEvent` / `BSTEventSink<BSAnimationGraphEvent>`。

### 3.7 Manager / Singleton（41 個 `*Manager` + 其他單例）
取得方式幾乎都是 `RE::Xxx::GetSingleton()`。

- **遊戲核心**：`Main`、`UI`、`Console` / `ConsoleLog`（印到主控台）、`Calendar`（遊戲時間）、`Sky`（天氣/光照）、`TESDataHandler`、`ProcessLists`、`SkyrimVM`（Papyrus VM）。
- **設定/輸入**：`GameSettingCollection`、`INISettingCollection`、`ControlMap`、`PlayerControls`、`MenuControls`、`UserEvents`、`BSInputDeviceManager`。
- **資源/系統**：`MemoryManager`、`BSAudioManager` / `BGSDefaultObjectManager`、`BSFaceGenManager`、`ImageSpaceManager`、`BSShaderManager`、`BSRenderManager`、`ModelReferenceEffect`、`BGSSaveLoadManager`、`BSScaleformManager`、`MenuTopicManager`、`SubtitleManager`、`CombatManager`、`ActorEquipManager`。

### 3.8 NetImmerse / NiObject（場景圖、3D 變換，86 個 `Ni*`）
- 基底：`NiObject`、`NiObjectNET`、`NiAVObject`、`NiNode`（節點，含子物件）、`NiGeometry`。
- 數學：`NiPoint3`、`NiPoint2`、`NiMatrix3`、`NiQuaternion`、`NiTransform`、`NiColor`、`NiBound`。
- 控制器/動畫：`NiControllerManager`、`NiControllerSequence`、`NiTimeController`。
- 智慧指標：`NiPointer<T>`、`NiSmartPointer`。
- 常見用途：找骨骼節點、掛載特效模型、改變物件位置/旋轉/縮放（搭配本專案 `util.h` 的 `NifUtil`）。

### 3.9 Havok 物理（144 個 `hk*` / `bhk*`）
`hkVector4`、`hkpWorld`、`hkpRigidBody`、`bhkWorld`、`bhkCharacterController`、
`hkpCharacterProxy`、`CFilter`（碰撞層過濾，`util.h::NifUtil::Collision` 有用到）。
一般 plugin 較少直接碰 Havok，除非要做碰撞/物理特效。

### 3.10 BS* 模板與工具（188 個）
- 容器：`BSTArray`（≈ `std::vector`）、`BSTHashMap`、`BSTList`、`BSTSmartPointer`、`BSFixedString`（interned 字串）、`BSString`。
- 事件框架：`BSTEvent`（`BSTEventSink` / `BSTEventSource`）。
- 並行/工作：`BSTSingletonSDM`、`BSTask`、`BSTempEffect`。
- 字串/序列化：`BSFixedString`、`BSScript`（Papyrus 綁定底層）。

---

## 4. REX — 平台底層
`REX/W32`（Win32 API 型別包裝，例如 D3D / DXGI / 視窗）、`REX/PS4`。一般 PC plugin 很少直接用。

---

## 5. 怎麼開始用（常見模式速記）

```cpp
// 取得玩家
auto* player = RE::PlayerCharacter::GetSingleton();

// 用 FormID 查 form（本專案 util.h 另有 FormUtil::Parse 處理 "formid~modname"）
auto* form = RE::TESForm::LookupByID(0x00012345);
auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(0x00012345);

// 透過 DataHandler 列舉某型別的所有 form
auto* dh = RE::TESDataHandler::GetSingleton();
for (auto* wep : dh->GetFormArray<RE::TESObjectWEAP>()) { /* ... */ }

// 訂閱事件
class HitSink : public RE::BSTEventSink<RE::TESHitEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* e,
        RE::BSTEventSource<RE::TESHitEvent>*) override { /* ... */ return RE::BSEventNotifyControl::kContinue; }
};
RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESHitEvent>(sink);

// 印到主控台
RE::ConsoleLog::GetSingleton()->Print("hello");

// 把工作丟回主執行緒（很多遊戲操作必須在主執行緒）
SKSE::GetTaskInterface()->AddTask([]() { /* ... */ });
```

> **小抄**：不確定某 class 有哪些方法時，直接打開對應 header（`RE/<首字母>/<ClassName>.h`）看；
> 跨版本函式位址用 `RELOCATION_ID(...)`；遊戲操作盡量包進 `GetTaskInterface()->AddTask` 在主執行緒做。

---

## 6. 重點 class 速查（常用方法 / 成員）

以下方法名稱皆取自實際 header（`RE/<首字母>/<ClassName>.h`），可直接打開該檔看完整簽名。

### `TESForm`（所有 form 的基底，`RE/T/TESForm.h`）
- 查找（**static**）：`LookupByID(FormID)`、`LookupByID<T>(FormID)`、`LookupByEditorID<T>("EditorID")`。
- 身分：`GetFormID()`、`GetFormType()`、`GetName()`、`Is(FormType)` / `IsNot(...)`。
- 轉型：`As<T>()`（失敗回 `nullptr`，等同安全 dynamic_cast）。
- 旗標：`IsPlayable()`、`IsDeleted()`、`SetFormID(...)`。

### `TESObjectREFR`（場景中的「實例參照」，`RE/T/TESObjectREFR.h`）
> form 是「定義」，refr 是「世界裡那一個實體」。`Actor`、放在地上的劍、門… 都是 refr。
- 生成：`PlaceObjectAtMe(base, bForcePersist)` — 在自己位置生一個 base 的實例（≈主控台 `placeatme`，最可靠的生成法）。
- 位置/朝向：`GetPosition()`、`SetPosition(NiPoint3)`、`GetAngle()`、`SetAngle(NiPoint3)`、`MoveTo(refr)`。
- 顯隱：`Enable(bResetInventory)`、`Disable()`、`Get3D()`（拿 NiNode 場景圖節點）、`DoReset3D()`。
- 物品欄：`GetInventory()`、`AddObjectToContainer(base, extraList, count, fromRefr)`、`RemoveItem(...)`、`GetContainer()`。
- 環境：`GetParentCell()`、`GetWorldspace()`、`GetBaseObject()`、`GetName()`、`GetFormID()`。

### `Actor` / `Character` / `PlayerCharacter`（`RE/A/Actor.h`、`RE/P/PlayerCharacter.h`）
> 繼承鏈：`Actor : Character : TESObjectREFR`，所以上面 refr 的方法 actor 都能用。
- 基底資料：`GetActorBase()`（回 `TESNPC*`，改外觀/名字/race 等）、`GetRace()`、`GetLevel()`。
- 法術：`AddSpell(SpellItem*)`、`RemoveSpell(...)`、`HasSpell(...)`。
- 裝備查詢：`GetEquippedObject(bLeftHand)`。實際裝備/卸除走 `ActorEquipManager`（見下）。
- 屬性值（繼承自 `ActorValueOwner`，`RE/A/ActorValueOwner.h`）：
  `GetActorValue(av)`、`GetBaseActorValue(av)`、`SetActorValue(av, f)`、`ModActorValue(av, f)`、`RestoreActorValue(...)`。
  `av` 用 `RE::ActorValue::kHealth` / `kMagicka` / `kStamina` / `kOneHanded` … 列舉。
- 生死：`IsDead()`、`KillImpl(...)`。
- 玩家專屬（PlayerCharacter）：`GetSingleton()`、相機、技能經驗、犯罪等大量成員。

### `ActorEquipManager`（`RE/A/ActorEquipManager.h`，singleton）
`EquipObject(actor, boundObj, ...)`、`UnequipObject(actor, ...)`、`EquipSpell(actor, spell, slot)`、`EquipShout(actor, shout)`。

### `SpellItem` / `MagicItem` / `EffectSetting`（魔法三件組）
- `SpellItem`（`RE/S/SpellItem.h`，繼承 `MagicItem`）：`GetSpellType()`、`GetCastingType()`、`GetDelivery()`、`CalculateMagickaCost(caster)`、`GetCostliestEffectItem()`、`effects`（`BSTArray<Effect*>`）。
  可寫成員：`data.spellType`、`data.castingType`、`data.delivery`（見 `MagicSystem::*` 列舉）。
- `Effect`（spell 內的單一效果條目）：`baseEffect`（指向 `EffectSetting`）、`effectItem.magnitude` / `.duration` / `.area`。
- `EffectSetting`（MGEF，魔法效果定義，`RE/E/EffectSetting.h`）：`data.archetype`（`EffectArchetypes::ArchetypeID::kScript` 等）、`data.flags`、`data.castingType`、`data.delivery`。

### `TESDataHandler`（`RE/T/TESDataHandler.h`，singleton — 查找/列舉所有 form）
- `LookupForm(localFormID, "Plugin.esp")` / `LookupForm<T>(...)` — 用「mod 內相對 ID + 檔名」查（跨載入順序安全，本專案 `FormUtil::Parse` 即包這個）。
- `LookupFormID(localFormID, "Plugin.esp")` — 拿到實際執行期 FormID。
- `GetFormArray<T>()` — 取得某型別**所有** form 的陣列（例如列舉全部武器 `GetFormArray<TESObjectWEAP>()`）。
- `CreateReferenceAtLocation(base, pos, angle, cell, worldspace, ...)` — 在指定座標生實例（比 `PlaceObjectAtMe` 更可控）。

### `IFormFactory`（`RE/I/IFormFactory.h` — 執行期動態建立 form）
`GetConcreteFormFactoryByType<T>()` 取得工廠，再 `factory->Create()->As<T>()`。
本專案用它在執行期動態造 `SpellItem` / `EffectSetting`，**完全不需要 ESP**（見 `MODDING_COOKBOOK.md`）。

### `UI` / `UIMessageQueue`（選單控制）
- `RE::UI::GetSingleton()` → `IsMenuOpen("MenuName")`、`GetMenu<T>()`。
- `RE::UIMessageQueue::GetSingleton()->AddMessage(Menu::MENU_NAME, UI_MESSAGE_TYPE::kShow / kHide, nullptr)` 開關選單。
- 每個選單 class 有 `MENU_NAME` 常數（如 `RaceSexMenu::MENU_NAME`）。

### `ConsoleLog`（`RE/C/ConsoleLog.h`，singleton）
`RE::ConsoleLog::GetSingleton()->Print("格式 %s", ...)` — 印到遊戲內 `~` 主控台，debug 很方便。

### `CrosshairPickData`（`RE/C/CrosshairPickData.h`，singleton）
`GetSingleton()->target[0].get()` — 玩家準星目前指向的 refr。本專案用來抓「玩家瞄準的東西」。
