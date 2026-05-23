# 程式碼審查報告 — Court Wizard 整合分支（quest-engine + procgen + co-save）

> 唯讀 QA / Code Review。**只報告，不修改程式碼**。
>
> **基準狀態**：已執行 `git merge feature/court-wizard`（fast-forward），HEAD = `169f779`「feat(gen-item): runtime procedural ITEM form generation + 'GITM' co-save rebuild」。審查的是最終整合樹。
>
> **建置驗證**：以 `cmake --preset build-release-clang-cl-linux` + `cmake --build` 在 Manjaro 上交叉編譯整合樹，**編譯與連結成功**，產出 `TemplatePlugin.dll`（1.7 MB），僅有無害的 clang-cl 旗標警告。整合後仍可連結。

---

## 整體健康度（Overall Health）

整體品質**偏高**：核心引擎（`src/core/`）嚴守可移植性（零 `RE::`/`SKSE::`），解析全面採容錯（不丟例外）、安全存取器到位、`validate()` 詳盡。Co-save 中央派發器（`CoSave.cpp`）正確地實作了「每個 plugin 只能一個 `SetUniqueID` + 一組 Save/Load/Revert」這個 SKSE 硬限制：全樹**只有一處** `SetUniqueID`/`SetSaveCallback`/`SetLoadCallback`/`SetRevertCallback`，四個 record type（`'QEST'`/`'GNPC'`/`'PRGN'`/`'GITM'`）互不衝突，`GetNextRecordInfo` 迴圈逐筆讀頭並依型別派發，符合契約。建置衛生良好（所有 `.cpp`/`.h` 皆登記、core 維持 `SKIP_PRECOMPILE_HEADERS`、fence 註解平衡、無 git 衝突標記、無重複入口點）。

但有**兩個會在實機造成問題的缺陷**需修：(C1) `SkyrimAdapter::OnRevert` 在**序列化執行緒**上呼叫 `engine_->start()`，對 court-wizard 任務會在非主執行緒觸發 `RE::DebugNotification`（違反主執行緒紀律，可能崩潰/UB）；(H1) `ProcgenItem::RebuildStaged` 每次讀檔都重新鑄造並加入物品，但**從不移除前一輪的物品**（標頭文件聲稱會 strip，實作沒有），導致每次讀檔玩家背包物品**累積疊加**。另有一個已記載但未防護的 ranged-weapon `RangedData*` 指標別名風險，目前僅靠預設模板是近戰武器而未爆發。

---

## 嚴重度彙總表（English summary table）

| # | Sev | File:line | Category | Issue (one-liner) |
|---|-----|-----------|----------|-------------------|
| C1 | Critical | `src/skyrim/SkyrimAdapter.cpp:261-278` | code bug | `OnRevert` calls `engine_->start()` on the serialization thread; for the court-wizard quest `on_start` runs `show_message` → `RE::DebugNotification` off the main thread (main-thread-discipline violation, possible crash/UB). |
| H1 | High | `src/skyrim/procgen/ProcgenItem.cpp:466-531` | code bug + in-game | Item rebuild re-mints + `AddObjectToContainer` but never strips the prior count; items **accumulate every reload**. Header (`ProcgenItem.h:36-38`) claims a strip that the impl does not do. |
| H2 | High | `src/skyrim/procgen/ProcgenItem.cpp:170` | code bug (latent) | `w->weaponData = tmpl->weaponData;` shallow-copies `Data` which holds `RangedData* rangedData`; a **ranged** template would alias one `RangedData` between minted weapon and template (double-free / shared-edit). Only safe because the default template is melee (rangedData null). |
| M1 | Medium | `src/core/QuestEngine.cpp:496-509` | code bug (edge) | In `submitChoice`, if a choice's `reset_quest` re-`start()`s and that `on_start` opens a NEW dialogue, the post-action guard unconditionally clears `activeDialogue`/`currentNode`, wiping the freshly-started dialogue. |
| M2 | Medium | `src/skyrim/SkyrimAdapter.cpp:147-176` + `plugin.cpp:80` | code bug (edge) | Engine `start()` runs once at kDataLoaded (intro `show_message` fires at main-menu time); then `importProgress` resets it at kPostLoadGame. On the load-save path the intro message is shown spuriously / out of context. |
| M3 | Medium | `src/skyrim/procgen/Procgen.cpp:489-519, 288-301` | code bug (fidelity) | Spell-path clutter loses its `scatter_aabb/count/seed` through `RecipeToJson` (PieceSpec drops them), so on rebuild `PlaceClutter` uses a degenerate AABB and drops clutter at local (0,0,0) instead of the authored positions. |
| M4 | Medium | `src/skyrim/SkyrimAdapter.cpp:196-201`, `SkyrimEvents.cpp:92-94` | design limit | Timers only advance via the F9 debug hotkey (no periodic poll); the court-wizard demo's `wr_summon` timer cannot fire in normal play without the debug key. Documented as deferred, but it means the demo cannot progress unaided. |
| M5 | Medium | `src/skyrim/procgen/ProcgenItem.cpp` (whole module) | in-game-validation | Native dynamic-form persistence of WEAP/ARMO/MISC + the "blank shell" reload behavior is community-evidence-based, not verified here; combined with H1 the net inventory result needs in-game confirmation. |
| L1 | Low | `src/skyrim/SkyrimAdapter.cpp` (no call) | robustness | `QuestEngine::validate()` is never invoked by the adapter; a malformed shipped quest would only surface as runtime log spam, not a load-time refusal/warning. |
| L2 | Low | `config/procgen/recipe_cottage.json`, `demo_procgen.json` | in-game-validation | `DefaultCandleLight01NS` / `Tankard01` EditorIDs are MEDIUM confidence (skip-with-warning if absent) — already self-flagged in JSON. |
| L3 | Low | `src/skyrim/procgen/ProcgenItem.h:36-38` vs `.cpp:481` | docs | Header doc and implementation comment contradict each other on whether dedup/strip happens (impl is correct, header is wrong). |
| L4 | Low | `src/plugin.cpp:43-46` | robustness | `OnDataLoaded` is reached at every kDataLoaded; `StartDemoQuest` rebuilds the engine each time but `BuildEngine` does not re-clear `globals_`/event sinks, so re-entry would double-install the activate/debug sinks (only matters if kDataLoaded fires more than once in a session). |

---

## Critical / High 詳述

### C1 — `OnRevert` 在序列化執行緒呼叫 `start()`，對含 `show_message` 的任務會在非主執行緒打 HUD（Critical）
`src/skyrim/SkyrimAdapter.cpp:261-278`

```cpp
void SkyrimAdapter::OnRevert(SKSE::SerializationInterface*) {
    ...
    if (self->engine_) {
        // start() ... This runs on the serialization thread but only mutates plain
        // std state inside the core (no RE:: live state) — same safety as OnSave.
        self->engine_->start();
    }
}
```

**問題**：註解的前提（「`start()` 只動 core 的純 std 狀態，不碰 `RE::`」）**不成立**。`start()` → `runStart()` → `runActions(on_start)`。`demo_court_wizard.json` 的 `on_start`（`config/quests/demo_court_wizard.json:9-12`）含 `{ "show_message": "..." }`，會走 `presenter_->showMessage()`（`QuestEngine.cpp:283`）→ `MessageBoxPresenter::showMessage` → `RE::DebugNotification(...)`（`MessageBoxPresenter.cpp:84-87`）。`OnRevert` 由 SKSE 在**序列化執行緒**呼叫（`CoSave.cpp:55` 的 `OnRevert` → 派發到此），因此這是在非主執行緒呼叫 `RE::DebugNotification`。同理，若任何任務的 `on_start` 含 `start_dialogue` 或 adapter 動作（`give_gold`/`spawn_character`…），會在序列化執行緒直接觸碰 `RE::` 活狀態。

**為何要緊**：違反整個 adapter 反覆宣稱的主執行緒紀律（`SkyrimActions.h:5-9`、`Procgen.h:14-19`、`CoSave.h:26-29`）。`RE::DebugNotification` 與 MessageBox/AddObjectToContainer 等都假設主執行緒，可能崩潰或 UB。

**建議**：`OnRevert` 不要呼叫 `start()`。改為只清掉 staged blob + globals + 標記引擎需重置；真正的重置/重啟延到下一個主執行緒時機（kPostLoadGame 的 `RebuildStaged`，或用 `OnMainThread` 包起來）。Procgen/Npc/Item 的 `OnRevert` 正確地只做 `Registry().clear()`（純記憶體），可作為對照範本。

### H1 — 生成物品每次讀檔累積疊加，且與標頭文件矛盾（High，code bug + 須實機確認）
`src/skyrim/procgen/ProcgenItem.cpp:466-531`、`ProcgenItem.h:36-38`

`ProcgenItem.h:36-38` 明文承諾：

> 「To avoid double-adding if a blank native form lingered, the rebuild path first **strips any prior tracked count of that logical item from the player before re-adding** the freshly minted one (best-effort; see ProcgenItem.cpp RebuildStaged).」

但 `RebuildStaged()` 實際只做：清空 registry → 逐筆 `MintItem` → `player->AddObjectToContainer(obj, nullptr, count, nullptr)`（`ProcgenItem.cpp:512`）。**全程沒有任何 `RemoveItem`/`GetItemCount` 去重**（已 grep 確認，`.cpp:481` 自己也寫「We do NOT attempt to dedupe」）。

**為何要緊**：vanilla 存檔編碼器會序列化玩家背包（含上一輪鑄造的 0xFF 動態物品基底）。讀檔時背包先被還原，接著 `RebuildStaged` 又鑄造一份新的並加入 → 每次讀檔玩家就**多 `count` 件**同名物品，反覆讀檔會無限累積。NPC 路徑沒有這問題，因為 `PlaceObjectAtMe(base, false)` 放的是暫時 ref，不入存檔；物品則是進背包、會被存。

**建議**：rebuild 前依 `key`/template 對玩家背包做 best-effort `RemoveItem`（如標頭所承諾），或改為偵測背包既有的同基底數量後只補差額；至少先把標頭與實作對齊（見 L3）。最終仍需實機確認 vanilla codec 對動態物品的還原行為（M5）。

### H2 — Ranged-weapon `RangedData*` 指標別名（High，潛伏）
`src/skyrim/procgen/ProcgenItem.cpp:170`

```cpp
w->weaponData = tmpl->weaponData;   // Data（DNAM）的記憶體逐成員淺拷貝
```

`RE::TESObjectWEAP::Data`（DNAM）內含 `RangedData* rangedData`（已對 `TESObjectWEAP.h:167` 確認）。淺拷貝後，鑄造出的武器與模板武器**共用同一個 `RangedData` 堆積物件**。若鑄造武器被釋放並釋放 `rangedData`，會破壞 vanilla 模板（double-free / use-after-free）；或修改鑄造武器的彈道資料會改到模板。

**現況**：`.cpp:168-169` 註解已自承此風險，並指出近戰模板 `rangedData` 為 null 所以「melee demo 沒事」。預設武器模板是 Iron Sword（近戰），故目前不爆發；但**任何 ranged 模板（弓/弩）的 recipe 都會踩到**。

**建議**：在 MintWeapon 後，若 `tmpl->weaponData.rangedData` 非 null，為鑄造武器深拷貝一份 `RangedData`（new + memcpy `sizeof(RangedData)==0x1C`）再指過去；或在文件層面明確禁止 ranged 模板直到深拷貝就位。

---

## Medium 詳述

### M1 — `submitChoice` 中 `reset_quest` 若重啟出新對話會被守衛清掉（Medium，core 邊界）
`src/core/QuestEngine.cpp:496-509`

```cpp
if (!pc.then.is_null()) runActions(pc.then);          // 可能執行 reset_quest
if (st_.terminated || st_.activeDialogue != dlgId) {  // 守衛
    st_.activeDialogue.clear();
    st_.currentNode.clear();
    return;
}
```

`reset_quest` → `applyInitialState()`（清空 `activeDialogue`）→ `runStart()`。若 `on_start` 內含 `start_dialogue`，`startDialogue` 會把 `activeDialogue` 設成新對話並已 `presentCurrentNode()`。回到守衛時 `st_.activeDialogue != dlgId`（新對話 id ≠ 舊 id）成立 → 無條件 `clear()`，**抹掉剛開始的新對話狀態**（但 presenter 已顯示，pending_ 也已建立 → 狀態不一致）。目前 `demo_court_wizard` 的 `thanks` 對話 `reset_quest` 後的 `on_start` 不開對話，所以未觸發；屬正確性潛伏缺陷。

**建議**：守衛應區分「reset/terminate 後 *沒有* 新對話」（才清）與「reset 後已開新對話」（保留）。例如僅在 `st_.activeDialogue == dlgId` 時才視為仍在舊對話而結束之；`activeDialogue` 變成別的非空值時不要 clear。

### M2 — 引擎在 kDataLoaded `start()`、kPostLoadGame 才 `importProgress`，導致開場訊息時機錯亂（Medium）
`src/skyrim/SkyrimAdapter.cpp:147-176`、`src/plugin.cpp:43-46,57-81`

`StartDemoQuest`（kDataLoaded，主選單載入時呼叫一次）會 `OnMainThread([]{ engine_->start(); })`，立即跑 `on_start`（顯示「你成為白漫的客座大法師…」+ 排 `wr_summon` timer）。之後讀存檔時 kPostLoadGame → `SkyrimAdapter::RebuildStaged` → `importProgress` → `applyInitialState()`（清掉剛排的 timer 與狀態）再套用存檔進度。結果：開場提示在「還在主選單 / 尚未載入存檔」時就閃出；timer 也先被排再被清。最終存檔狀態正確，但有可見的時機/語境錯亂。

**建議**：kDataLoaded 只 `BuildEngine`（建引擎、不 `start()`）；新遊戲（kNewGame）才 `start()`，讀存檔（kPostLoadGame 且有 staged）才 `importProgress`。如此「開新局」與「讀存檔」兩條路徑分流，不會搶跑 on_start。

### M3 — 法術路徑生成的房間，clutter 重建後跑到原點（Medium，重建保真度）
`src/skyrim/procgen/Procgen.cpp:489-519`（`RecipeToJson`/`pieceToJson`）、`Procgen.cpp:199-238`（`PlaceClutter`）

法術路徑（`OnGenerateRoom`）的 clutter `scatter_aabb/count/seed` 來自 recipe 檔，經 `ParseRecipe`→`ParsePiece` 後**只存進 `PieceSpec` 的 base/位置/旋轉，不存 scatter 參數**。`RecipeToJson`/`pieceToJson` 再從 `PieceSpec` 重建 clutter JSON，因此**丟失** `scatter_aabb/count/seed`。重建時 `doc.contains("clutter")` 為真 → 傳給 `PlaceInterior`→`PlaceClutter`，但每筆 `count` 預設 1、`scatter_aabb` 不存在 → lo=hi={0,0,0} 的退化 AABB → `dx/dy/dz(rng)=0` → clutter 被放到 local (0,0,0)，而非首次生成時的 authored 位置（首次走 `Procgen.cpp:293-299` 的 authored fallback）。adapter/JSON 路徑因 `clutterDoc` 被原樣存進 recipe（`GenerateInterior` `Procgen.cpp:554`）故不受影響。

**建議**：把 clutter 的 scatter 參數（aabb/count/seed）也保存進 `PieceSpec` 或在 `RecipeToJson` 時為 clutter 另存原始 scatter 欄位，讓法術路徑與 adapter 路徑的重建一致。

### M4 — Timer 僅靠 F9 推進，demo 任務無法在正常遊玩中前進（Medium，設計限制）
`src/skyrim/SkyrimAdapter.cpp:196-201`、`src/skyrim/SkyrimEvents.cpp:92-94,114-125`

`checkTimers()` 只由 F9 debug hotkey 呼叫，沒有週期輪詢。`demo_court_wizard` 的整條流程仰賴 `wr_summon`（48 小時後）這個 timer 才會送信開對話。實機若不按 F9，任務不會自行推進。原始碼已標註為 Phase 1 故意延後，屬已知限制，但意味著「demo 無法自走」。

**建議**：實機驗證計畫中明確標註需 F9；長期應接一個低成本的週期 tick（如每遊戲分鐘或 OnSleepStop/卡片事件）去呼叫 `CheckTimers`。

---

## 須實機驗證（不能在無 Skyrim 環境下確認）

- **M5 / H1 配套**：vanilla 存檔對 WEAP/ARMO/MISC 動態基底的還原行為（是否回來變空殼、背包是否保留），決定 H1 累積疊加的實際嚴重度與正確修法。
- **L2**：`DefaultCandleLight01NS`、`Tankard01` EditorID 是否解析（JSON 已標 MEDIUM、解析失敗會 skip-with-warning，不崩潰）。
- **procgen 幾何**：家具/長椅相對圓桌的精確位移、床睡眠標記可達性、`flatten_to_max` 結果（JSON `_todo` 已列）。
- **預設模板 FormID**：`0x0001BCD8`（Bandit，NPC）、`0x00012EB7`/`0x00012E49`/`0x0000000F`（Iron Sword/Iron Armor/Gold001，Item）標為 VERIFIED-common，仍建議實機 `help` 確認。

---

## 確認為「正確 / 無虞」的重點（避免誤判）

- **Co-save 單一註冊**：全樹只有 `CoSave.cpp:87-90` 一處 `SetUniqueID`/三個 callback；四個 record type 互異；`OnLoad`（`CoSave.cpp:33-52`）以 `GetNextRecordInfo` 逐筆讀頭、依 4-char 型別派發，handler 只讀自身 payload（`'GNPC'`/`'PRGN'`/`'GITM'`/`'QEST'` 的 `OnLoad` 皆收 `(version,length)` 而**不自行消頭**，符合契約）。Revert 對所有 handler fan-out。`SerializationInterface` 全為 const 方法，故 `Register(const ...*)` 合法。
- **Core 零 RE::/SKSE::**：`QuestEngine.{h,cpp}`/`QuestState.h`/`Ports.h` 僅用 std + nlohmann-json；`sourcelist.cmake` 對 `core_sources` 設 `SKIP_PRECOMPILE_HEADERS ON`（`CMakeLists.txt:50`）。
- **容錯解析**：`QuestEngine` 全程用 `jsonAt`/`tryGetString`/`tryGetNumber` 安全存取器，非物件/缺鍵/型別不符皆記錄並安全略過，**不丟例外**；`importProgress`/`restoreGlobals` 對壞 blob 逐欄降級、回 false 而非崩潰。
- **Timer 絕對時間語意**：`exportProgress` 存絕對 game-hours，`checkTimers` 以 `t <= now` 比較單調遞增的 `Calendar::GetHoursPassed()`，跨存檔正確。
- **建置衛生**：所有新 `.cpp`/`.h` 已登記於 `sourcelist.cmake`/`headerlist.cmake`；`// >>>`/`// <<<` 與 cmake `# >>>`/`# <<<` fence 全數平衡；無 `<<<<<<<`/`=======`/`>>>>>>>` 衝突標記；唯一 `SKSEPluginLoad`、無 `SKSEPluginInfo` 重複。
- **Handle 生命週期**：`SkyrimEntities::resolved_` 用 `ActorHandle`（`Actor::CreateRefHandle()` 確為 `ActorHandle`）；ProcgenNpc 用 `ObjectRefHandle`（可由 `Actor*` 經 `BSPointerHandle` 模板建構子轉換）。皆以 `.get()` 檢查 stale。
- **`As<T>` vs `static_cast`**：ProcgenItem 對 component base（`TESValueForm`/`TESWeightForm`/`TESAttackDamageForm`）正確改用 `static_cast`（`As<T>` 只認 concrete form type，會回 null），對 concrete form（WEAP/ARMO/MISC）用 `As<T>`；ARMO 走真正的 `Copy` override、WEAP/MISC 走 component `CopyComponent`，與標頭推理一致。
- **Procgen 重建去重守衛**：`GenerateInterior/Structure` 先 `ClearGenerated(key)` 再放置；`RebuildStaged` 先 `Rooms().clear()` 再重建；rooms 的 ref 是 `forcePersist=false` 暫時 ref，不入存檔，故無 item 那種累積問題。`flatten_to_max` 結果烘進 `origin.z` 且重建時 `ground` 改 `none`，不會重複加偏移。
- **MessageBox 非同步回呼**：`MessageBoxData.callback` 由選單持有（`make_smart`），`ChoiceCallback::Run` 的 `Message` 值即 0-based 按鈕索引，經 sink → `SubmitChoice` → `OnMainThread` → `submitChoice`，按鈕索引映射正確；終端節點以無回呼的 OK box 呈現。
