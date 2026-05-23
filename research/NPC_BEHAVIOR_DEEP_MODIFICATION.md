# Skyrim NPC 行為修改深度分析（SKSE / CommonLibSSE-NG）

> 探索性可行性分析，2026-05-23。由 research agent 產出，所有引擎面論斷均對照本 repo vendored 的
> `CommonLibSSE-NG/include/RE/` 標頭查證。**僅分析，未動 plugin 程式碼。**
> 與 `NAVMESH_FREE_PATHFINDING.md`（位移層）、`3D_PHYSICAL_ALCHEMY_FEASIBILITY.md` 為同批研究。

## 前提：兩個被混用的系統

「行為樹最底層，尤其是 Radiant AI」其實混用了兩個**完全不同**的系統：

- **「AI 行為」** ＝ AI Package / Procedure 系統（決策層：NPC「要做什麼」——巡邏、睡覺、戰鬥、沙盒）。Radiant AI / Radiant Quest 屬這一層。
- **「動畫行為」** ＝ Havok Behavior Graph（執行層：決定好要做什麼後，**身體怎麼動**——播哪個動畫、狀態機轉換）。Nemesis / Pandora / FNIS / DAR / OAR 全在這一層。

兩者在引擎裡是分開的子系統，能改的程度與手段差很多。以下由高到低分四層。

---

## 第 1 層：AI Packages / Radiant AI（決策層）

**核心類別與位置**
- `RE::TESPackage`（`T/TESPackage.h`）：一個 AI 包的「定義」（form）。關鍵成員：`packData`（`PACKAGE_DATA`，含 `packType`、flags）、`procedureType`（`PACKAGE_PROCEDURE_TYPE` enum，offset 0xD8）、`packConditions`（`TESCondition`）、`packSched`（排程）、`ownerQuest`（offset 0x70——Radiant Quest 指派包的掛勾點）、`combatStyle`、`packTarget`。
- `enum class PACKAGE_PROCEDURE_TYPE`（同檔 line 34）：`kFind / kFollow / kEscort / kSleep / kWander / kTravel / kSandbox / kGuard / kDialogue / kCastMagic / kFlee …` 共 ~46 種——這就是「procedure type」。
- `RE::ActorPackage`（`A/ActorPackage.h`）：actor 身上「**目前正在跑**的包」實例。成員：`package`（→`TESPackage`）、`currentProcedureIndex`、`packageStartTime`、`packageLock`（`BSSpinLock`，改前要鎖）。
- 包選擇/評估入口都在 `Actor`（`A/Actor.h`）：
  - `Actor::EvaluatePackage(bool a_immediate, bool a_resetAI)`（line 523）——**強制重新評估包堆疊**。
  - `Actor::GetCurrentPackage()`（line 536）——讀目前的包。
  - `Actor::PutCreatedPackage(TESPackage*, bool tempPackage, bool createdPackage, bool allowFromFurniture)`（vtable index **0xDF**，line 421）——**把自製包塞給 actor**。
  - 語意化注入入口：`InitiateFlee(...)`（0xDD）、`InitiateDialogue(...)`（0xD8）、`InitiateGetUpPackage()`、`InitiateDoNothingPackage()`、`StopCombat()`（0xE5）、`SetCombatGroup(CombatGroup*)`（0xD5）。
- `Actor::CheckForCurrentAliasPackage()`（vtable 0x049, line 291）——quest alias（含 Radiant）指派包的查詢點。

**Procedure「樹」**：引擎內部確實有 procedure-tree 類別——`I/IProcedureTreeItem.h`、`B/BGSProcedureTreeProcedure.h`、`BGSProcedureTreeBranch.h`、`BGSProcedureTreeSequence.h`、`BGSProcedureTreeConditionalItem.h`。但**在 CommonLibSSE-NG 裡幾乎全是 `Unk_XX` stub**（`IProcedureTreeItem` 只有 `Load` 一個具名 vfunc）。也就是說：package 內部那棵真正的「程序樹」雖被逆向出存在，**但沒有可用的讀寫 API**——無法從 C++ 有意義地重建或編輯這棵樹。

**runtime 可讀 vs 可寫**
- 可讀：目前的包、procedure index、條件、排程、target——全可讀。
- 可寫：`TESPackage` 成員是公開的，技術上可改，但這是改「全域 form 定義」，會影響所有用此包的 NPC，且不入存檔。
- 注入：用 `IFormFactory`（`MODDING_COOKBOOK.md` R4 已有此模式）動態造 `TESPackage`，設好 `procedureType`/flags/target，呼叫 `PutCreatedPackage(...)` 塞給特定 actor，最後 `EvaluatePackage(true)` 強制生效。**這是最乾淨的「客製單一 NPC 行為」路徑。**

> ⚠️ `skyrim_mod/answers/NPC_NPC_Dynamic_Work_Behavior_Architecture.md` 裡寫的 `currentProcess->PushPackage(...)` **在 headers 裡不存在**（skyrim_mod 本就是「可能有錯的教學材料」——已查證確認此處有誤）。正確做法是 `Actor::PutCreatedPackage` + `EvaluatePackage`，或走 Faction PackList / quest alias 指派。

**風險/工時**：中等。API 都在、概念清楚，但 `TESPackage` 結構複雜（target/location/schedule/condition 互相關聯），動態造一個功能完整的包要試錯。完全不需外部工具。

---

## 第 2 層：AI Process / High-Process（狀態層）

**核心類別與位置**
- `RE::AIProcess`（`A/AIProcess.h`）：經 `actor->GetActorRuntimeData().currentProcess` 取得（offset 0xF0）。
  - 分層 `enum PROCESS_TYPE { kHigh / kMiddleHigh / kMiddleLow / kLow }`；成員 `high`（`HighProcessData*`）、`middleHigh`、`middleLow`、`processLevel`。
  - `currentPackage`（`ActorPackage`，offset 0x18）——即第 1 層那個跑中的包。
  - 方法：`GetRunningPackage()`、`GetCurrentShout()`、`GetHeadtrackTarget()` / `SetHeadtrackTarget(...)`、`PlayIdle(...)`、`StopCurrentIdle(...)`、`SetupSpecialIdle(...)`、`GetCommandingActor()`。
  - 成員 `followTarget`、`target`、`lowProcessFlags`（`kAlert / kFollower / kPackageDoneOnce …`）、`equippedObjects`。
- `RE::HighProcessData`（`H/HighProcessData.h`，size 0x478）：`animSequencer`、`currentShout`、`headTrackTarget[6]`/`headTracked[6]`（依 `HEAD_TRACK_TYPE`）、`currentMovementType`、pathing 向量（`pathingDesiredPosition`/`pathingDesiredOrientation`/`pathingCurrentMovementSpeed`…）、`fadeState`、`crimeToReactTo`、`greetingPlayer`、`inCommandState`、`detectionModifier` 等。
- `RE::MiddleHighProcessData`（`M/MiddleHighProcessData.h`，size 0x338）：`runOncePackage`、`currentPackageSpell`（`MagicItem*`，offset 0x240——NPC 包指定要施的法術）、`commandedActors`、`charController`、`currentMovementSpeed`、`preventCombat`、`isFleeing`、`hostileGuard`、骨節指標。
  - **關鍵橋接點**：`animationGraphManager`（`BSTSmartPointer<BSAnimationGraphManager>`，offset 0x1A8）與 `animationVariableCache`（offset 0x1B0）——第 2 層通往第 3 層的入口。
- `Actor` runtime data（`A/Actor.h` line 644 `ACTOR_RUNTIME_DATA`）：`currentProcess`、`currentCombatTarget`（`ActorHandle`, 0xFC）、`myKiller`、`actorMover`（0x140）、`movementController`、`combatController`（0x158）、`emotionType`/`emotionValue`、`magicCasters[]`、`selectedSpells[]`、`selectedPower`。
- `RE::ProcessLists`（`P/ProcessLists.h`，singleton）：`ForAllActors(...)`/`ForEachHighActor(...)` 批次走訪；旗標 `runDetection/processHigh/runMovement/runAnimations` 全域開關；`StopCombatAndAlarmOnActor(...)`。批量改行為從這入手。

**runtime 可讀 vs 可寫**：大量欄位是裸 public，技術上可讀可寫；但很多是引擎每幀重算的快取（cached values、pathing 向量），寫進去常被下一幀覆蓋。穩定可控的多半是「透過官方方法」設定（`SetHeadtrackTarget`、`StopCombat`），而非直接寫成員。

**風險/工時**：中～高。讀取很安全（適合做行為偵測/觸發）；寫入要注意執行緒（必須主執行緒，`SKSE::GetTaskInterface()->AddTask`）、要鎖、易被引擎覆寫。直接亂寫 `unkXXX` 成員 = crash。

---

## 第 3 層：Havok Behavior Graph（動畫執行層，真正的「behavior tree / 狀態機」）

這層才是 Skyrim 真正意義上的「behavior tree」——但**它不是 C++ 物件樹，而是 `.hkx` 檔裡的資料**。

**核心類別與位置**
- `RE::IAnimationGraphManagerHolder`（`I/IAnimationGraphManagerHolder.h`）：**每個 `TESObjectREFR`（含所有 Actor）都繼承它**（`T/TESObjectREFR.h` line 112）。主要、安全的操作介面：
  - `NotifyAnimationGraph(const BSFixedString& eventName)`（vtable 0x01）——**送一個動畫事件進圖**。
  - `SetGraphVariableFloat / Int / Bool / NiPoint3(name, value)` 與 `GetGraphVariable...`——**讀寫圖變數**（`bIsBlocking`、`Speed`、`Direction`、`IsNPC`…）。
  - `GetAnimationGraphManager(BSTSmartPointer<BSAnimationGraphManager>&)`。
- `RE::BSAnimationGraphManager`（`B/BSAnimationGraphManager.h`）：管一個 actor 的多張圖。`graphs`（`BSTSmallArray<BShkbAnimationGraph>`）、`variableCache`（`BSAnimationGraphVariableCache`，含 `AnimVariableCacheInfo{ name, hkbVariableValue* }`——可看到變數名與值指標）。本身是 `BSTEventSink<BSAnimationGraphEvent>`。
- `RE::BShkbAnimationGraph`（`B/BShkbAnimationGraph.h`）：單張 Bethesda 包裝的圖。**直接提供 `GetGraphVariableBool/Float/Int` 與 `SetGraphVariableBool/Float/Int`**（內含 `RELOCATION_ID`，例 SetFloat = `RELOCATION_ID(63608, 62709)`）。成員 `behaviorGraph`（`hkbBehaviorGraph*`, 0x208）、`projectName`、`holder`、`rootNode`（`BSFadeNode`）。同時是 `BSTEventSource<BSAnimationGraphEvent>`，提供 `AddEventSink<T>`/`RemoveEventSink<T>`——**可掛 sink 監聽圖事件**。
- `RE::hkbBehaviorGraph`（`H/hkbBehaviorGraph.h`）：**Havok 那棵狀態機/行為圖本體**。但成員幾乎全是 `hkRefVariant`（不透明）：`rootGenerator`、`data`、`variableValueSet`、`variableIDMap`、`activeNodes`…
- `RE::hkbStateMachine`（`H/hkbStateMachine.h`）：`StateInfo`/`TransitionInfoArray` **幾乎全是 `unkXX`**（逆向不完整）。
- `RE::BSAnimationGraphEvent`（`B/BSAnimationGraphEvent.h`）：`{ tag, holder, payload }`——監聽到的動畫事件長這樣。

**SKSE 在這層能做 vs 不能做（問題核心）**
- ✅ **能做**：
  - `NotifyAnimationGraph("EventName")` 對 actor 送動畫事件。
  - `SetGraphVariable*`/`GetGraphVariable*` 驅動/讀取圖變數（驅動既有狀態機的轉換條件）。
  - `Actor::AddAnimationGraphEventSink(BSTEventSink<BSAnimationGraphEvent>*)`（`A/Actor.h` line 498）——掛 sink **攔截每一個動畫事件**（footstep、attackStart…）。
  - `IsAnimationDriven()`、`GetSequencer()` 等讀取狀態。
- ❌ **不能做（從 C++）**：
  - **重建/修改圖的拓樸**——新增狀態、改 transition、加 generator 節點、改節點連線。狀態機真正結構是 `.hkx` 二進位資料（`rootGenerator`/`data` 是 `hkRefVariant` 不透明物件），CommonLib 沒有、也幾乎不可能提供安全編輯 API。
  - 「想讓 NPC 做引擎沒有的全新動作流程」**必須先有對應 `.hkx` 行為圖支援**。

**這層要靠外部工具**（已查證）：行為圖用 **Havok Behavior Tool**（官方中介軟體）建/編輯，再用 `hkxcmd`/`hkx2` 解打包 `.hkx`。要把自訂狀態**注入** vanilla 圖，用 **Nemesis / Pandora**（`Monitor221hz/Pandora-Behaviour-Engine-Plus`，與本專案 vcpkg registry 同作者）/ **FNIS**——做的是「graph injection」。**DAR / OAR（Open Animation Replacer）** 是另一回事：不改圖拓樸，而是在「圖要播某動畫」時依條件替換實際播出的動畫檔；OAR 本身是 SKSE 插件並開放 condition 外掛 API——若目標是「同一行為、不同情況播不同動畫」，OAR 是正解，不必自寫 hook。

**風險/工時**：驅動圖變數/送事件/掛 sink = 低～中風險，純 C++、API 齊全；改圖拓樸 = SKSE C++ 做不到，屬外部工具 + 美術/動畫工程，工時極高。

---

## 第 4 層：Hooking points（攔截「行為決策」）

本專案現況：`src/hook.cpp`/`.h` 是空 stub；`src/util.h` 只用 `REL::Relocation<func_t>{ RELOCATION_ID(seID, aeID) }` **呼叫**遊戲函式（如 `AnimUtil::Idle::Play` 用 `RELOCATION_ID(38290, 39256)`），**尚未做任何 trampoline branch-hook**。SKSE trampoline API 在 `SKSE/Trampoline.h`（`write_branch<N>`/`write_call<N>`/`create`/`allocate`）。

**務實攔截手段（由易到難）**
1. **事件 sink（最安全，優先選）**：不需 hook 位址。
   - `BSTEventSink<BSAnimationGraphEvent>` + `Actor::AddAnimationGraphEventSink(...)`：攔截動畫事件流（第 3 層）。
   - `ScriptEventSourceHolder` 的 `TESPackageEvent`（`COMMONLIBSSE_INDEX.md` 3.6 已列）：監聽包開始/結束/變更——零風險觀察 Radiant AI 換包。配合 `TESCombatEvent`、`TESFurnitureEvent`。
2. **vtable hook（中等）**：`TESObjectREFR` 是多型，CommonLib 標好 vtable index，可換掉某 actor class 的虛擬函式指標：
   - `Actor::Update(float)`（vtable **0x0AD**, `A/Actor.h` line 371）／`UpdateAnimation`（0x07D）：每幀進入點（效能敏感）。
   - `IAnimationGraphManagerHolder::NotifyAnimationGraph`（vtable 0x01）：攔截/改寫送進圖的事件。
   - `Actor::CheckForCurrentAliasPackage`（0x049）、`PutCreatedPackage`（0xDF）：攔截包指派本身。
   - 做法：取 `REL::Relocation<std::uintptr_t> vtbl{ VTABLE_Actor[0] }`，備份原指標再覆寫；`VTABLE_Actor` 已含 SE/AE/VR 位址。
3. **函式位址 branch-hook（最強最脆）**：對「包評估函式」本體下 `write_branch`。參考 `kassent/SkyrimSouls` 的 `Hook_Game.cpp`。

**位址告誡**（呼應 `CLAUDE.md`）：所有 `RELOCATION_ID(seID, aeID)` 的 ID **必須自己從 Address Library 逐版本查**。vtable index 由 CommonLib 維護相對安全；裸函式位址跨 SE/AE/VR 必須各查一組，查錯即 crash。記得包進主執行緒並處處檢查 null。

---

## (a) 分層總表：能改什麼、怎麼改 vs 需要外部工具 / 不可行

| 層 | 想做的事 | SKSE C++ 可行性 | 手段 |
|---|---|---|---|
| 1. AI Package | 讀目前包 / 強制重評估 | ✅ 易 | `GetCurrentPackage()`、`EvaluatePackage(true)` |
| 1. AI Package | 給單一 NPC 注入自製包（巡邏/沙盒/施法…） | ✅ 中 | `IFormFactory` 造 `TESPackage` → `PutCreatedPackage(...)` → `EvaluatePackage` |
| 1. AI Package | 編輯 package 內部 procedure **樹**結構 | ❌ 不可行 | 類別是 `Unk_XX` stub，無 API |
| 1. Radiant | 觀察/反應 Radiant 換包 | ✅ 易 | `TESPackageEvent` sink |
| 2. AI Process | 讀戰鬥目標 / head-track / 移動狀態 / emotion | ✅ 易 | `currentProcess`、`HighProcessData`、`MiddleHighProcessData` 成員 |
| 2. AI Process | 改戰鬥目標 / 停戰 / 設 head-track / 設包法術 | ✅ 中（主執行緒+鎖） | `StopCombat()`、`SetHeadtrackTarget()`、`SetCombatGroup()`、`ProcessLists` |
| 2. AI Process | 直接寫每幀重算的快取欄位 | ⚠️ 多半被覆寫 | 不建議 |
| 3. Behavior Graph | 驅動圖變數 / 送動畫事件 / 監聽事件 | ✅ 易～中 | `NotifyAnimationGraph`、`Set/GetGraphVariable*`、`AddAnimationGraphEventSink` |
| 3. Behavior Graph | **改狀態機拓樸 / 加新狀態與轉換** | ❌ C++ 不可行 | 須 Havok Behavior Tool + Nemesis/Pandora/FNIS（外部 `.hkx`） |
| 3. 動畫 | 同行為依條件換不同動畫 | ✅（用現成插件） | DAR / **OAR**（含 condition API），不必自寫 hook |
| 4. Hook | 攔截行為決策/動畫事件 | ✅ 中（位址逐版查） | event sink ＞ vtable hook（`Actor::Update` 0xAD 等）＞ 裸函式 branch-hook |

**一句話**：SKSE C++ 能在**決策層（包）**做「換包/造包/重評估」、在**狀態層**讀寫戰鬥/移動/朝向、在**動畫層**「驅動變數＋送事件＋監聽事件」；但**不能**從 C++ 重建任何「樹」——無論 package 的 procedure tree 還是 Havok 的 behavior graph 拓樸，後者一律靠外部 `.hkx` 工具鏈。

## (b) 建議的下一步研究路徑

1. **先確定目標屬於哪一層**：「NPC 該去做什麼」→ 第 1 層 Package，先做；「NPC 做引擎沒有的新動作」→ 第 3 層，先評估是否真需新 `.hkx`，還是 OAR 換動畫就夠。
2. **零風險起手式（純觀察）**：照 `MODDING_COOKBOOK.md` R7 加一個 `TESPackageEvent` sink + 一個 `BSAnimationGraphEvent` sink，把每個 NPC 的「換包」與「動畫事件」全寫進 log。先看懂 vanilla NPC 一天的包與事件序列。
3. **驗證 Package 注入**：用 `IFormFactory` 造一個簡單 `kSandbox`/`kTravel` 包，對準星指到的 NPC 呼叫 `PutCreatedPackage` + `EvaluatePackage(true)`。最小可行驗證，符合本專案「純 C++、不靠 ESP」核心模式。
4. **驗證圖變數驅動**：對 actor 呼叫 `NotifyAnimationGraph("IdleStop")` 或 `SetGraphVariableBool("bAnimationDriven", true)`，確認能影響動畫狀態，再決定要不要碰 `.hkx`。
5. **若確定要動 behavior graph**：研究 **Pandora** 的 graph injection 格式與 wiki，以及 `hkxcmd`/`hkx2` 解包流程——這條路是動畫工程，不是 C++ 插件工程。
6. **Hook 前置**：研究 Address Library，為 SE/AE/VR 各查一組 offset；vtable hook 先於裸函式 hook。參考 `kassent/SkyrimSouls` 的 `Hook_Game.cpp`。
7. **更正本地教學**：`skyrim_mod/answers/NPC_NPC_Dynamic_Work_Behavior_Architecture.md` 的 `PushPackage` 不存在，動手前以 `Actor::PutCreatedPackage` 為準。

**最相關標頭**：`RE/T/TESPackage.h`、`RE/A/ActorPackage.h`、`RE/A/AIProcess.h`、`RE/A/Actor.h`（`EvaluatePackage` 523 / `PutCreatedPackage` 421 / `Update` 371 / `ACTOR_RUNTIME_DATA` 644）、`RE/H/HighProcessData.h`、`RE/M/MiddleHighProcessData.h`（`animationGraphManager` 0x1A8）、`RE/P/ProcessLists.h`、`RE/I/IAnimationGraphManagerHolder.h`、`RE/B/BSAnimationGraphManager.h`、`RE/B/BShkbAnimationGraph.h`、`RE/B/BSAnimationGraphEvent.h`、`RE/H/hkbBehaviorGraph.h`、`RE/H/hkbStateMachine.h`、`RE/I/IProcedureTreeItem.h`（procedure 樹，僅 stub）、`SKSE/Trampoline.h`、`src/util.h`、`src/hook.cpp`。

**Sources**
- [Pandora-Behaviour-Engine-Plus (Monitor221hz)](https://github.com/Monitor221hz/Pandora-Behaviour-Engine-Plus) / [Wiki](https://github.com/Monitor221hz/Pandora-Behaviour-Engine-Plus/wiki)
- [D Merge — json-based hkx patcher](https://www.nexusmods.com/skyrimspecialedition/mods/152190)
- [Open Animation Replacer (OAR)](https://www.nexusmods.com/skyrimspecialedition/mods/92109) / [DAR](https://www.nexusmods.com/skyrimspecialedition/mods/33746) / [dargh (open-source DAR)](https://github.com/noxsidereum/dargh)
- [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG)
- [SkyrimSouls Hook_Game.cpp](https://github.com/kassent/SkyrimSouls/blob/master/SkyrimSouls/Hook_Game.cpp)
- [Creation Kit — AI Package](https://steamcommunity.com/groups/SkyrimCKPublic/discussions/1/620712999967169136/)
