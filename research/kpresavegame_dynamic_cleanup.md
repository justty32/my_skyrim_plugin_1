# 存檔前移除動態 (0xFF) actor/ref 的安全設計

> 目標：在遊戲**寫入 `.ess` 之前**，把我們在 session 內鑄造（mint）出來的動態（`0xFF......`）
> actor / ref 全部移除，讓 `.ess` 不再持久化「懸空的動態 base」，從而根治
> **跨 session 冷重啟讀檔 crash**（log 停在 `OnRevert` 之後、沒有 `OnLoad`，引擎在還原
> `.ess` 動態 actor 時崩潰）。within-session 讀檔複製問題已由 `ProcgenNpc.cpp` 的
> 「re-adopt（重新接管 codec 還原的 actor）」路徑解決，本文**不動那條路**。
>
> 本文僅為設計，所有簽章皆對照本 repo 內 vendored 的真實 SKSE / CommonLibSSE-NG 標頭驗證，
> 不憑記憶。實作者請依「§7 實作計畫」逐項落地。

---

## 0. 一個必須先講清楚的硬事實：本 SKSE **沒有 `kPreSaveGame`**

任務簡報假設有一個 `SKSE::MessagingInterface::kPreSaveGame` 訊息。**這個 repo 的 SKSE 沒有它。**
已對照 `CommonLibSSE-NG/include/SKSE/Interfaces.h:280-293` 的列舉，存檔相關訊息只有 **`kSaveGame`**：

```cpp
// CommonLibSSE-NG/include/SKSE/Interfaces.h:280-293  (逐字)
enum : std::uint32_t
{
    kPostLoad,        // 0
    kPostPostLoad,    // 1
    kPreLoadGame,     // 2
    kPostLoadGame,    // 3
    kSaveGame,        // 4   <-- 唯一的存檔訊息；沒有 kPreSaveGame
    kDeleteGame,      // 5
    kInputLoaded,     // 6
    kNewGame,         // 7
    kDataLoaded,      // 8
    kTotal
};
```

`grep -rn "kPreSaveGame\|PreSaveGame"` 在整個 `CommonLibSSE-NG/include` 與 `src` 也只命中
`kSaveGame`，**完全沒有 `kPreSaveGame`**。所以「在 `kPreSaveGame` handler 移除」這個原始計畫**字面上無法成立**。
重點是改用正確的機制達到同樣效果，下面 §1/§2 解釋。

> 結論先講：**正確的「存檔前清理鉤子」應該掛在 co-save 的 `SetSaveCallback`（也就是我們
> 既有的 `skyrim::cosave::OnSave` 中央分派器）裡**，而不是任何 message。理由見 §1/§2。

---

## 1. SKSE 存檔訊息與 co-save Save callback 的真實時序

### 1.1 兩個機制、兩個簽章（皆已對照標頭）

**(A) MessagingInterface `kSaveGame` 訊息** — `Interfaces.h:262-315`：

```cpp
// CommonLibSSE-NG/include/SKSE/Interfaces.h:265-273, 310-311  (逐字)
struct Message {
    const char*   sender;
    std::uint32_t type;        // 例如 kSaveGame
    std::uint32_t dataLen;
    void*         data;        // kSaveGame: 指向存檔名字串（const char*）
};
using EventCallback = void(Message* a_msg);
bool RegisterListener(const char* a_sender, EventCallback* a_callback) const;
```

我們的 listener 是 `src/plugin.cpp` 的 `void MessageHandler(SKSE::MessagingInterface::Message*)`，
已用 `messaging->RegisterListener("SKSE", MessageHandler)` 註冊（`plugin.cpp:104-108`）。

**(B) SerializationInterface Save callback** — `Interfaces.h:81-99`：

```cpp
// CommonLibSSE-NG/include/SKSE/Interfaces.h:84, 94-99  (逐字)
using EventCallback = void(SerializationInterface* a_intfc);
void SetUniqueID(std::uint32_t a_uid) const;
void SetSaveCallback(EventCallback* a_callback) const;   // <-- 存檔回呼
void SetLoadCallback(EventCallback* a_callback) const;
void SetRevertCallback(EventCallback* a_callback) const;
```

我們**已經**透過中央分派器掛了這個 Save callback：`skyrim::cosave::Register` 呼叫
`intfc->SetSaveCallback(OnSave)`（`CoSave.cpp:88`），而 `cosave::OnSave` 會 fan-out 給每個
模組的 `save` handler（`CoSave.cpp:22-27`）。

### 1.2 它們相對於「真正序列化」與彼此的觸發時點

引擎存檔流程（社群實證 + 本 repo `research/PROCGEN_NPC_FORMS.md` §5「時機」一致）大致是：

```
玩家觸發存檔 (BGSSaveLoadManager::Save，RE/B/BGSSaveLoadManager.h:87)
        │
        ├─[1] 引擎序列化 .ess（寫入所有 changeform，含動態 0xFF actor 的 changeform）
        │
        ├─[2] SKSE 寫入伴隨的 .skse co-save
        │        └── 此時呼叫我們的 SerializationInterface Save callback
        │            （cosave::OnSave -> 各模組 OnSave，OpenRecord/WriteRecordData）
        │
        └─[3] SKSE 派發 MessagingInterface kSaveGame 訊息 (data = 存檔名)
                 └── 此時呼叫我們的 MessageHandler(kSaveGame)
```

**關鍵時序結論（決定本設計的正確掛點）：**

1. **`kSaveGame` 訊息在 `.ess` 已經寫完之後才到。** 在 `MessageHandler` 收到 `kSaveGame` 時，
   動態 actor 的 changeform **早就進 `.ess` 了**，這時再 `Disable()/SetDelete()` 對「這一次的
   存檔」沒有任何意義（已寫入的 `.ess` 不會回頭改）。所以**不能**靠 `kSaveGame` message 做存檔前清理。
   （它仍可作為「下一輪保險」，但那只是延後一個存檔週期，不是根治。）

2. **co-save 的 Save callback（`cosave::OnSave`）是在 SKSE 寫 `.skse` 那一刻被呼叫的。**
   它**不是嚴格地早於 `.ess` 序列化**——`.ess` 與 `.skse` 是兩個檔，引擎先寫 `.ess` 才換 SKSE 寫
   `.skse`。所以在 `OnSave` 裡刪 actor，**也救不了「這一次」已寫好的 `.ess`**。

> ⚠️ 這代表「在 Save callback / kSaveGame 訊息裡移除 actor」都**無法影響當次 `.ess`**。
> 真正能「在 `.ess` 寫入前」介入的點，是 **`BGSSaveLoadManager::Save` 本身的函式入口 hook**
> （見 §3「方案 C」），那是唯一保證早於 `[1]` 的點。但它需要 address-library / RELOCATION_ID
> （我們沒有現成 ID），且風險高。請讀完 §2/§3 再決定。

3. **執行緒**：co-save 三個 callback（Save/Load/Revert）跑在 **SKSE 的序列化執行緒，不是主執行緒**
   —— 這是 `CoSave.h:26-29` 與 `research/PROCGEN_NPC_FORMS.md` §5 反覆強調的鐵律。
   `kSaveGame` 訊息**也不保證在主執行緒**。**`Disable()` / `SetDelete()` 觸碰 live RE:: 狀態，
   在序列化執行緒上呼叫是危險的**（與 `OnRevert` 不可碰 RE:: 同理，見 `research/REVIEW_FINDINGS.md`
   C1 的修法）。這把「在哪個窗口刪 actor 才安全」變成本題的核心難點 → §2/§3。

---

## 2. 為什麼「存檔前刪掉動態 actor」這個方向仍然是對的，以及它的 round-trip

### 2.1 round-trip 確認：刪了還能讀回來嗎？——能

我們的持久化是**邏輯食譜重建**，不是引擎級 FormID 身分（`ProcgenNpc.h:24-30`、`Procgen.h:31-36`、
`ProcgenItem.h:27-46`）。每個模組存進 co-save 的是 **recipe + 模板的既有 plugin FormID + 位置**，
讀檔時 `RebuildStaged()` 在 `kPostLoadGame` 主執行緒**重新 mint**。**我們從不存 `0xFF` 動態 base/ref id。**

因此，只要 **co-save 的 `OnSave` 仍然把 recipe 寫進去**，那麼即使我們把 live 動態 actor 從世界裡刪掉，
讀檔時的 `RebuildStaged()` 依然會**依食譜重造一份**。round-trip 成立：

```
存檔時：co-save 寫 recipe（'GNPC'/'PRGN'/'GITM'）  + 我們移除 live 0xFF actor/ref
              ↓
.ess 不再含懸空動態 base（理想狀況，見 §2.2 的時序限制）
              ↓
讀檔時：OnLoad stage recipe -> kPostLoadGame RebuildStaged 重新 mint
              ↓
同一個「邏輯 NPC / 房子 / 物品」重新出現
```

> 換言之，**移除動態 actor 與 co-save 重建是互補的，不衝突**：recipe 是 source of truth，
> live ref 只是 session-local 的呈現。刪掉 live ref **不會**讓 co-save 少寫 recipe，
> 只要刪除動作**發生在 `OnSave` 寫完 recipe 之後 / 或根本不碰 registry**（見 §2.3 雙刪風險）。

### 2.2 「在 kPostLoadGame 刪會 crash，在存檔窗口刪會不會也 crash？」

`progress.md:31` 與 `ProcgenNpc.cpp:457-489` 記錄了血淚教訓：
**對 codec 還原的 actor 在 `kPostLoadGame` 窗口（含延遲到 `Is3DLoaded`）做 `Disable()/SetDelete()` 是致命的**，
所以 NPC 改用 re-adopt 不刪。

但**存檔窗口（gameplay 進行中、玩家 3D 已穩定載入）刪 actor 的安全性，與 kPostLoadGame 窗口不同**：

- `kPostLoadGame` 窗口危險，是因為 actor 是「codec 剛還原、process manager / 動畫圖 / 3D 尚未 settle」的
  半成品（`ProcgenItem.cpp:443-448` 的註解、`progress.md:31-32`）。
- **存檔通常發生在 gameplay 穩定時**（手動存檔、自動存檔），actor 是「我們自己這個 session mint、已正常
  運行」的 ref，`Disable()/SetDelete()` 是 `Generate()` 同 session 重鑄時就在用、且實測 OK 的操作
  （`ProcgenNpc.cpp:271-277`、`Procgen.cpp:256-270` `DropRoom`、`ProcgenItem` `RemoveItem`）。

> 所以本題的安全性論點是：**「在一個 gameplay-穩定的主執行緒 tick 上刪我們自己 mint 的 actor」是安全的；
> 危險的從來是「在剛讀檔、半還原的窗口上刪 codec 還原的 actor」。** 兩者必須分清。
>
> ⚠️ 但 §1.2 已指出：`kSaveGame` 訊息與 co-save `OnSave` callback **都不保證在主執行緒**，
> 也都晚於 `.ess` 寫入。要同時滿足「主執行緒 + 早於 .ess」只有 §3 方案 C（hook `Save`）。
> 因此本文給出**分層方案**（§3），並標明各自的 trade-off 與風險。

### 2.3 與 re-adopt 路徑、雙刪的互動（必須避免）

`ProcgenNpc::RebuildStaged` 的 re-adopt 分支（`ProcgenNpc.cpp:467-489`）會在讀檔後**重新接管**
那顆還活著的動態 actor 並放回 registry。若本設計在「下一次存檔」把它刪掉、又**沒有保留 recipe**，
就會兩邊打架。避免方式：

- **存檔清理只刪 live ref，不清空 registry、不刪 recipe**（recipe 留著給下次讀檔重建）。
- **不要在序列化執行緒上同時 mutate registry 與 RE:: 狀態**。
- re-adopt 是讀檔路徑、本清理是存檔路徑，兩者時間上不重疊（一個在 `kPostLoadGame`、一個在存檔），
  只要各自只動自己份內的 handle，不會雙刪。
- 同 session 重存多次：每次清理後 live ref 已 `SetDelete`，handle 失效；下次清理時
  `handle.get()` 回 null，自然 no-op，不會二次刪。

---

## 3. 三個可落地的方案（依「正確性 vs. 風險」排序）

### 方案 A（推薦、低風險、近乎根治）：在 co-save `OnSave` 內，**先寫 recipe、再排一個主執行緒 task 刪 live ref**

`cosave::OnSave`（序列化執行緒）裡：每個模組的 `save` handler 照常 `OpenRecord + WriteRecordData`
（recipe 進 `.skse`），**然後**把「移除 live 動態 ref」的動作用
`SKSE::GetTaskInterface()->AddTask(...)` 丟到主執行緒（`Interfaces.h:196` `void AddTask(TaskFn)`）。

- ✅ 寫 recipe 與排 task 都在序列化執行緒做，但**實際的 `Disable()/SetDelete()` 在主執行緒 tick 跑**
  （避開「序列化執行緒碰 RE::」的雷，符合 C1 修法精神）。
- ✅ round-trip 成立：recipe 已寫入 `.skse`。
- ⚠️ **限制（誠實話）**：task 在主執行緒跑的時間點，**晚於當次 `.ess` 寫入**。所以
  **「這一次」存的 `.ess` 仍然含動態 actor changeform**；真正乾淨的是「**下一次**存檔」。
  也就是說方案 A 把問題**延後一個存檔週期**消除，而不是當次根治。
  - 但實務上這仍**大幅降低 crash 機率**：玩家通常會在同一 session 多次存檔/自動存檔，
    第二次起的存檔就乾淨；且配合方案 B 可在「即將關遊戲前」主動清一次。
- ⚠️ 仍要小心：`AddTask` 的 lambda **只能捕捉 FormID（by value），絕不能捕捉 `TESObjectREFR*`
  或 handle 跨 frame**（`ProcgenItem.cpp:505-510` 的鐵律）。task 內用 `TESForm::LookupByID` 重解析，
  確認 `>= 0xFF000000` 且仍是我們 registry 裡記的那顆，才刪。

### 方案 B（推薦搭配 A）：在 `MessageHandler` 的 `kSaveGame` case 也排一次主執行緒清理

`plugin.cpp` 的 `switch` 加 `case SKSE::MessagingInterface::kSaveGame:`，呼叫各模組的
`StripAllMintedForSave()`（見 §5）。

- ✅ `kSaveGame` 是個明確的「玩家剛存了檔」訊號，適合做「清掉這 session 的動態殘留、讓下一次存檔乾淨」。
- ✅ 與方案 A 等效（都是「延後一週期」），但語意更清楚、且不佔用序列化執行緒。
- ⚠️ 同樣**不影響當次 `.ess`**（§1.2 結論 1）。`kSaveGame` 不保證主執行緒，故清理動作仍應透過
  `AddTask` 丟主執行緒再執行 RE:: 操作。

> **建議的最小落地組合 = A 或 B 擇一（推薦 B，因為不碰序列化執行緒、語意清楚）**。
> 兩者皆「延後一週期」，但對「冷重啟讀舊檔 crash」已是巨大改善：只要玩家在加入本 mod 後
> 存過 ≥1 次「乾淨」檔，那個檔就能跨 session 安全讀回。

### 方案 C（根治當次、但高風險，列為 TODO）：hook `BGSSaveLoadManager::Save` 函式入口

唯一能保證「早於 `.ess` 寫入」的點，是 hook 存檔函式本身：
`RE::BGSSaveLoadManager::Save(const char*)`（`RE/B/BGSSaveLoadManager.h:87`）或其
`Save_Impl`（同檔 `:190`）。在 trampoline hook 的「呼叫原函式之前」同步移除動態 actor，
再呼叫原函式去寫 `.ess`。

- ✅ 當次 `.ess` 就乾淨（真正根治、不延後）。
- ❌ **需要 address-library offset / RELOCATION_ID**：本 repo `src/hook.{h,cpp}` 是空殼
  （`CLAUDE.md`「Hook IDs / address-library offsets ... still have to be found manually」），
  **我們沒有 `Save` / `Save_Impl` 的 ID**。**不要憑空捏造 ID**。
- ❌ hook 在哪個執行緒跑取決於誰呼叫 `Save`；`BGSSaveLoadManager` 有自己的非同步存檔
  thread（`BGSSaveLoadManager.h:54-72` `Thread` / `asyncSaveLoadOperationQueue`），
  在那條 thread 上同步刪 actor 很可能比序列化執行緒更危險。
- 📌 **TODO（給人類 / 後續）**：
  - [ ] 找 `BGSSaveLoadManager::Save` 或 `Save_Impl` 的 SE/AE RELOCATION_ID（兩個 runtime 都要）。
  - [ ] 確認 hook 觸發執行緒；若非主執行緒，仍需 marshalling 或改用 `ProcessEvent(BSSaveDataEvent)` 等
        更高層、且在主執行緒的訊號（待查證，勿假設）。
  - [ ] 評估「同步刪 actor 後再讓引擎序列化」是否會讓引擎在序列化一個剛被 `SetDelete` 的 ref 時崩潰
        （`SetDelete` 後該 frame 內 ref 可能仍在某些清單上）。

> **建議**：先上方案 B（零 ID、零 hook、低風險），實測「存一次乾淨檔後冷重啟讀檔不再 crash」。
> 只有當「延後一週期」在實務上不可接受時，才投入方案 C 的 RELOCATION_ID 逆向。

---

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

## 8. 開放風險與 TODO 清單

- ⚠️ **「延後一週期」限制（方案 A/B 共有）**：當次 `.ess` 在 cleanup task 跑之前就寫好了，
  所以**第一個含召喚物的存檔仍可能 crash 於冷重啟**；要乾淨需「存過 ≥1 次後續檔」。
  若需要當次根治 → 方案 C。請在實作 commit message / progress.md 明確標註此特性，避免誤判為失敗。
- ⚠️ **`SetDelete` 後同 frame 的序列化交互**：方案 C 若 hook `Save` 入口、在序列化前同步刪 actor，
  需驗證引擎不會在序列化一個剛 `SetDelete` 的 ref 時崩潰。**未驗證**，列 TODO。
- ⚠️ **執行緒**：`kSaveGame` 訊息 / co-save `OnSave` 皆不保證主執行緒——本設計一律經 `AddTask`
  丟主執行緒再做 RE:: 操作。實作時**勿**在 callback 直接 `Disable()/SetDelete()`。
- ⚠️ **lambda 捕捉**：`AddTask` 的 lambda 只能捕 FormID（by value），task 內 `LookupByID` 重解析；
  **絕不**捕 `TESObjectREFR*` / handle 跨 frame（`ProcgenItem.cpp:505-510` 鐵律）。
- 📌 **TODO（方案 C，勿憑空填）**：找 `BGSSaveLoadManager::Save`（`BGSSaveLoadManager.h:87`）或
  `Save_Impl`（`:190`）的 SE/AE RELOCATION_ID（兩 runtime 都要）；確認 hook 觸發執行緒；
  `src/hook.{h,cpp}` 目前是空殼，須先建 trampoline。**沒有 ID 前不要寫 C 方案。**
- 📌 **ProcgenItem 是否要納入清理**：物品族（WEAP/ARMO/MISC）會被引擎序列化但重載成空殼，
  非 crash 主因（`ProcgenItem.h:27-46`）；納入清理是「為一致 / 防累積」，非根治 crash 必需。可後置。
- 📌 **NpcGenerator registry**：若採 Step 2，注意 `SpawnNpc` 目前無 mutex、無追蹤；新增的
  `g_spawned` 只在主執行緒（spell cast sink / cleanup task）被碰，理論上不需鎖，但與其他模組一致起見
  建議仍加 `std::mutex`。

---

## 9. 引用到的真實標頭與程式碼位置（供實作者核對）

- `CommonLibSSE-NG/include/SKSE/Interfaces.h`：`MessagingInterface` 列舉 `280-293`（**無 kPreSaveGame，僅 kSaveGame=4**）、
  `Message`/`EventCallback` `265-273`、`RegisterListener` `310-311`；`SerializationInterface`
  `81-99`（`SetSaveCallback`/`SetUniqueID`/`OpenRecord`/`WriteRecordData`/`ResolveFormID`）；
  `TaskInterface::AddTask` `196`。
- `CommonLibSSE-NG/include/RE/T/TESObjectREFR.h`：`SetDelete` `234`（vfunc 23）、`Disable` `337`
  （`SKYRIM_REL_VR_VIRTUAL`，vfunc 89）、`IsDisabled` `442`。
- `CommonLibSSE-NG/include/RE/B/BGSSaveLoadManager.h`：`Save(const char*)` `87`、`Save_Impl` `190`、
  非同步存檔 `Thread` `54-72`。
- `src/plugin.cpp`：`MessageHandler` `41-97`、`SKSEPluginLoad` 內 co-save 註冊 `110-129`。
- `src/skyrim/CoSave.{h,cpp}`：中央分派器；`OnSave` fan-out `CoSave.cpp:22-27`、`Register`/`SetSaveCallback`
  `CoSave.cpp:80-94`、執行緒規則 `CoSave.h:26-29`。
- `src/skyrim/procgen/ProcgenNpc.cpp`：`Registry()` `58-61`、`TrackedNpc.ref/priorRefFormID` `42/52`、
  同 session 重鑄拆除 `271-277`、re-adopt 分支 `467-489`、`OnSave` 寫 recipe `301-336`、
  `OnRevert` `405-412`、`RebuildStaged` `427-536`。
- `src/skyrim/procgen/Procgen.cpp`：`Rooms()` `54-57`、handle 向量 `44-46`、**`DropRoom` `256-270`**、
  `ClearGenerated` `767-786`、`OnSave` `822-847`。
- `src/skyrim/procgen/ProcgenItem.cpp`：`Registry()`/`TrackedItem.live` `78-91`、
  **`StripPriorInstances` `382-436`**、`AddObjectToContainer` `624`、`RebuildStaged` `729-808`、
  lambda/FormID 鐵律 `505-510`。
- `src/NpcGenerator.cpp`：`SpawnNpc`（無追蹤）`60-99`、`SpellCastHandler` `212-256`。
- `research/PROCGEN_NPC_FORMS.md`：§3.2 序列化型別清單、§5「時機」co-save round-trip、
  `IsDynamicForm()`=`>=0xFF000000` `325`。
- `progress.md:31`（kPostLoadGame 刪 codec actor 致命 → re-adopt 修法）、`:40`（跨 session crash
  未解、`C++: Spawn NPC` 主嫌、需 `kPreSaveGame` 鉤子根治）、`:46`（下一步列為 `kPreSaveGame` 清理）。
