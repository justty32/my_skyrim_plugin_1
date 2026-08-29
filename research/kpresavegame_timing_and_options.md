# 存檔清理：SKSE 時序與方案比較

[返回總索引](kpresavegame_dynamic_cleanup.md)

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
