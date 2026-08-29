# 存檔清理：清理範圍與實作計畫

[返回總索引](kpresavegame_dynamic_cleanup.md)

## 4. 怎麼**只**列舉「我們自己造的」動態 actor/ref（絕不誤刪別人的 form）

我們在三個 registry + 一個無 co-save 的 NpcGenerator 中追蹤了 mint 出來的東西。逐一說明可枚舉來源：

### 4.1 `skyrim::procgen::npc`（`ProcgenNpc.cpp`）——召喚 NPC
- registry：`Registry()` = `std::unordered_map<std::string, TrackedNpc>`（`ProcgenNpc.cpp:58-61`），
  受 `g_mutex` 保護。
- 每個 `TrackedNpc` 有 `RE::ObjectRefHandle ref`（`ProcgenNpc.cpp:42`）= live 動態 ref 的 handle，
  以及 `priorRefFormID`（`:52`）= 該 ref 當前的 `0xFF` FormID。
- **枚舉方式**：鎖 `g_mutex`，遍歷 `Registry()`，對每個 entry 取 `it->second.ref.get()`；
  若 `refr` 非 null **且** `refr->GetFormID() >= 0xFF000000`（確認是動態、是「我們這顆」），才處理。
  這正是 re-adopt 分支（`:470`）與重鑄分支已在用的 guard。

### 4.2 `skyrim::procgen`（`Procgen.cpp`）——程序生成的房間 / 城堡 ref
- registry：`Rooms()` = `std::unordered_map<std::string, GeneratedRoom>`（`Procgen.cpp:54-57`）。
- 每個 `GeneratedRoom` 有三組 handle 向量：`shellHandles` / `furnitureHandles` / `otherHandles`
  （`Procgen.cpp:44-46`）。已有現成的拆除函式 **`DropRoom(GeneratedRoom&)`（`Procgen.cpp:256-270`）**：
  對每個 handle `if (auto refr = h.get()) { refr->Disable(); refr->SetDelete(true); }`，然後 `clear()`。
- **枚舉方式**：直接重用 `DropRoom` 對每個 room 的 handle 操作；這些 ref 都是
  `CreateReferenceAtLocation(..., forcePersist=false, ...)` 放出來的動態 ref（`Procgen.cpp:158-160`）。

### 4.3 `skyrim::procgen::item`（`ProcgenItem.cpp`）——程序生成的物品 base（在玩家背包）
- registry：`Registry()` = `std::unordered_map<std::string, TrackedItem>`（`ProcgenItem.cpp:88-91`）。
- 物品**不是世界裡的 ref**，而是 mint 出來、`AddObjectToContainer` 進玩家背包的動態 **base form**
  （`TrackedItem::live`，`ProcgenItem.cpp:84`）。它的移除已有現成邏輯 **`StripPriorInstances`
  （`ProcgenItem.cpp:382-436`）**：用 `player->RemoveItem(live, count, ...)` 移除 live base，
  並掃背包移除「同 form type + 動態 id + 同名」的殘留。
- **枚舉方式**：對每個 `TrackedItem`，用 `StripPriorInstances(player, t.kind, name, t.live, t.count)`
  從玩家背包移除。**注意**：物品的「跨 session crash」風險本來就比 NPC/ref 小（`ProcgenItem.h:27-46`
  說明 WEAP/ARMO/MISC 會被序列化但重載成空殼，而非 crash 來源），但為一致性仍可一併清理。

### 4.4 `NpcGenerator`（`NpcGenerator.cpp`）——`C++: Spawn NPC` 沒有 co-save 的召喚
- ⚠️ **這是跨 session crash 的主要嫌犯之一**（`progress.md:40` 點名「`C++: Spawn NPC`（NpcGenerator,
  無 co-save）」）。`SpawnNpc`（`NpcGenerator.cpp:60-99`）`factory->Create()<TESNPC>` + `Copy` +
  `PlaceObjectAtMe`，**完全沒有 registry、沒有追蹤任何 handle**，spawn 完就丟掉指標。
- **問題**：目前**無法枚舉**這些 ref——我們沒記下它們。要清理，必須先**加一個極簡 registry**
  （存 `RE::ObjectRefHandle`）到 `NpcGenerator`，在 `SpawnNpc` 成功 spawn 後 `push_back` 進去。
  - 因為 `NpcGenerator` 沒有 co-save、本來就靠 `kNewGame`/`kPostLoadGame` 重給 spell（`plugin.cpp:53`），
    這些 spawn 出來的 NPC **本來就不打算持久**，所以「存檔前刪掉、不重建」對它們是正確語意
    （它們不像 procgen 那樣有 recipe 可重建——刪了就是刪了，符合「動態造的東西不進存檔」原則）。

### 4.5 共同的安全護欄（**絕不誤刪**）
- **只動 registry / handle 向量裡記著的東西**——這天然保證「只碰我們造的」。
- 動手前再加一道 `GetFormID() >= 0xFF000000` 檢查（`TESForm::IsDynamicForm()` 等義，
  `research/PROCGEN_NPC_FORMS.md:325` 提及 `IsDynamicForm()` = `>=0xFF000000`）：
  萬一 handle 因 ResolveFormID 之類原因指到了非動態 form，**跳過不刪**。
- handle 失效（`.get()` 回 null）→ no-op，自然安全。

---

## 5. 各模組應新增的對外函式（建議簽章）

每個模組新增一個「存檔前移除自己 mint 的動態內容」的函式，由 §7 的 `MessageHandler(kSaveGame)`
（方案 B）統一呼叫。命名統一為 `StripAllMintedForSave()`。

```cpp
// src/skyrim/procgen/ProcgenNpc.h —— 新增
// 移除本 session mint 的所有動態召喚 NPC ref（不清 registry、不刪 recipe —— recipe 留給
// 下次讀檔由 RebuildStaged 重建）。必須在主執行緒呼叫。
void StripAllMintedForSave();

// src/skyrim/procgen/Procgen.h —— 新增
// 移除本 session 程序生成的房間/城堡 ref（重用 DropRoom 的 handle 拆除；
// 同樣保留 Rooms() 與 recipe 給讀檔重建）。主執行緒。
void StripAllMintedForSave();

// src/skyrim/procgen/ProcgenItem.h —— 新增（可選；物品非 crash 主因）
// 從玩家背包移除本 session mint 的物品 base（重用 StripPriorInstances）。主執行緒。
void StripAllMintedForSave();

// src/NpcGenerator.h —— 新增（需先補一個 spawn handle registry，見 §4.4）
namespace NpcGenerator {
    void StripAllSpawnedForSave();   // 移除無 co-save 的 C++: Spawn NPC 召喚物
}
```

### 5.1 各 `StripAllMintedForSave` 的內部行為要點

- **ProcgenNpc**：鎖 `g_mutex`，遍歷 `Registry()`，對每個 `it->second.ref.get()`：
  `if (refr && refr->GetFormID() >= 0xFF000000) { refr->Disable(); refr->SetDelete(true); }`。
  **不要 `Registry().clear()`**（保留 recipe 行為由 `OnSave` 負責；registry 內的 recipe 字串
  也保留，讓 re-adopt/重建仍有依據）。可選擇把 handle 設為失效以免下次重刪。
- **Procgen**：鎖 `g_mutex`，`for (auto& [k, room] : Rooms()) DropRoom(room);`
  （`DropRoom` 已做 `Disable()+SetDelete(true)+clear handles`）。**不要 `Rooms().clear()`**
  —— 留著 `recipeJson`/`origin`/`seed`/`cellFormID` 等 co-save 欄位給讀檔重建。
- **ProcgenItem**：鎖 `g_mutex`，對每個 `TrackedItem` 呼叫
  `StripPriorInstances(player, t.kind, recipeName, t.live, t.count)`（recipeName 從 `t.recipeJson`
  parse 出 `"name"`）。
- **NpcGenerator**：遍歷新加的 spawn-handle 向量，`Disable()+SetDelete(true)`，清空向量。

> 全部假設在**主執行緒**被呼叫（由 §7 的 `AddTask` 保證）。每個函式內仍照慣例 null-check
> `PlayerCharacter::GetSingleton()` / handle。

### 5.2 用到的 RE:: API（皆已對照標頭）
- `RE::TESObjectREFR::Disable()` — `RE/T/TESObjectREFR.h:337`，`SKYRIM_REL_VR_VIRTUAL void Disable(); // 89`
  （VR 是 virtual、SE/AE 非 virtual；既有程式碼 `Procgen.cpp:261`/`ProcgenNpc.cpp:273` 已在用，無 ID 需求）。
- `RE::TESObjectREFR::SetDelete(bool)` — `RE/T/TESObjectREFR.h:234`，vfunc `// 23`（既有程式碼已用）。
- `RE::TESObjectREFR::GetFormID() >= 0xFF000000` — 動態判定（`research/PROCGEN_NPC_FORMS.md:325`）。
- `RE::PlayerCharacter::RemoveItem(...)` — `ProcgenItem.cpp:413` 既有用法。
- `SKSE::GetTaskInterface()->AddTask(TaskFn)` — `Interfaces.h:196`。

---

## 6. 為什麼這條路不會破壞既有的「讀檔重建」與「re-adopt」

| 路徑 | 觸發時機 | 動作 | 與本設計關係 |
|---|---|---|---|
| `cosave::OnSave` | 存檔寫 `.skse` 時（序列化執行緒） | 寫 recipe（'GNPC'/'PRGN'/'GITM'） | **保留不動**——recipe 是重建 source of truth |
| 本設計 `StripAllMintedForSave` | `kSaveGame`（方案 B）→ AddTask 主執行緒 | 刪 live 動態 ref，**不刪 recipe** | 新增；只刪呈現，留 recipe |
| `OnLoad` | 讀檔（序列化執行緒） | stage recipe | 不動 |
| `RebuildStaged`（NPC re-adopt / 重鑄） | `kPostLoadGame` 主執行緒 | re-adopt 還活的 ref，否則依 recipe 重鑄 | 不動；本設計只在「存檔」端動，時間不重疊 |

關鍵不變式：**recipe 永遠是 source of truth；live ref 只是 session 呈現。**
存檔端刪 live ref（讓 `.ess` 乾淨）+ 讀檔端依 recipe 重建 = 完整 round-trip，互不干擾。

---

## 7. 實作計畫（逐檔、逐函式）

> 推薦先做**方案 B**（零 RELOCATION_ID、零 hook、低風險）。方案 A 為可選加強，方案 C 列 TODO。

### Step 1 — 各 procgen 模組新增 `StripAllMintedForSave()`
- `src/skyrim/procgen/ProcgenNpc.{h,cpp}`：宣告 + 實作（§5.1）。實作放在 anonymous-namespace 之外的
  `namespace skyrim::procgen::npc`，內部鎖 `g_mutex`、遍歷 `Registry()`。
- `src/skyrim/procgen/Procgen.{h,cpp}`：同上，內部 `for (auto& [k,room]:Rooms()) DropRoom(room);`。
  （`DropRoom` 已是 file-local；`StripAllMintedForSave` 與它同檔，可直接呼叫。）
- `src/skyrim/procgen/ProcgenItem.{h,cpp}`（可選）：遍歷 `Registry()` 呼叫 `StripPriorInstances`。

### Step 2 —（若要清 NpcGenerator 的無追蹤召喚）給 `NpcGenerator` 補一個 spawn registry
- `src/NpcGenerator.cpp`：加一個 file-static `std::vector<RE::ObjectRefHandle> g_spawned;`（含 mutex）。
  在 `SpawnNpc`（`:78` `auto spawned = ...`）成功後 `g_spawned.push_back(RE::ObjectRefHandle(spawned->As<RE::TESObjectREFR>()))`。
- `src/NpcGenerator.h` / `.cpp`：新增 `void StripAllSpawnedForSave();`，遍歷 `g_spawned`
  `Disable()+SetDelete(true)`，清空。
- ⚠️ 若決定**不**清 NpcGenerator（例如先只處理有 co-save 的 procgen），可跳過 Step 2，
  但 `progress.md:40` 指出 `C++: Spawn NPC` 是跨 session crash 主嫌之一，**強烈建議納入**。

### Step 3 — 在 `src/plugin.cpp` 的 `MessageHandler` 加 `kSaveGame` case（方案 B 的掛點）
- 位置：`switch (a_msg->type)` 內、`kPostLoadGame` case 之後（`plugin.cpp:95` 之後）。
- 內容（示意，實作者填）：

```cpp
case SKSE::MessagingInterface::kSaveGame:
    SKSE::log::info("kSaveGame: save written; scheduling dynamic-actor cleanup");
    // 不在此處同步刪（kSaveGame 不保證主執行緒）；丟主執行緒 tick 執行 RE:: 操作。
    if (auto* task = SKSE::GetTaskInterface()) {
        task->AddTask([]() {
            skyrim::procgen::npc::StripAllMintedForSave();
            skyrim::procgen::StripAllMintedForSave();
            skyrim::procgen::item::StripAllMintedForSave();  // 可選
            NpcGenerator::StripAllSpawnedForSave();          // 若做了 Step 2
        });
    }
    break;
```

- `plugin.cpp` 已 include 全部需要的標頭（`ProcgenNpc.h`/`Procgen.h`/`ProcgenItem.h`/`NpcGenerator.h`，
  見 `plugin.cpp:2-17`），無新增 include 需求。

### Step 4 —（可選，方案 A）在各模組 `OnSave` 末尾也排一次清理 task
- 若覺得方案 B 不夠，可在 `ProcgenNpc::OnSave` / `Procgen::OnSave` / `ProcgenItem::OnSave`
  末尾 `if (auto* t = SKSE::GetTaskInterface()) t->AddTask([]{ StripAllMintedForSave(); });`。
  與方案 B 等效（延後一週期），**不要兩個都加導致重複排程**——擇一。

### Step 5 — 註冊新原始碼 / 標頭到 CMake
- 本步驟**只改了既有檔案**，沒有新增 `.cpp`/`.h`，**不需要**動 `cmake/sourcelist.cmake` /
  `cmake/headerlist.cmake`（`CLAUDE.md`「Adding source files」規則只在新增檔案時適用）。

### Step 6 — 測試（人類，Proton/Wine + MO2）
1. 全新遊戲 → 召喚一個 NPC（`C++: Spawn NPC` 與 procgen 召喚各一）+ 程序生成一間房。
2. **存檔兩次**（第一次可能仍含殘留；第二次應乾淨——方案 B/A 延後一週期特性）。
3. **完全關閉遊戲**，重開，讀**第二個**存檔。
4. 預期：不再於 `OnRevert` 後 crash；procgen NPC/房依 recipe 重建出現；`C++: Spawn NPC` 召喚物
   不重建（符合無 co-save 語意）也不 crash。
5. 看 log 確認 `kSaveGame: ... cleanup`、各模組 `Strip...` 訊息、讀檔 `RebuildStaged` 訊息。

---
