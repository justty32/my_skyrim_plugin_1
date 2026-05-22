# JSON 劇情/對話引擎 — 設計文件

> 狀態：**設計階段，尚未實作**。本檔是給之後實作的人（含 AI）與餵 LLM 生成劇情用的契約。
> 搭配閱讀：`MODDING_COOKBOOK.md`（本專案核心模式）、`COMMONLIBSSE_INDEX.md`（有哪些 class）、`PITFALLS.md`（編譯雷區）、`CLAUDE.md`（專案規則）。

## 0. 目標與一句話

用**純 JSON 定義劇情與對話**，執行期由 C++ 驅動，**完全不依賴 Creation Kit / ESP**。延續本專案「動態 form + 事件分派 + co-save 持久化」的核心模式。

範圍（已拍板）：
- 對話呈現：**原生 `DialogueMenu` 是高風險 spike；MessageBox 是保底**。呈現層可抽換。
- 劇情深度：**完整任務系統**（目標、世界觸發、追蹤）。
- NPC 來源：**現有 NPC 與動態生成都要**。
- 撰寫者：**手寫 + LLM 生成** → schema 要嚴格、詞彙封閉、錯誤好懂。

---

## 1. 第一鐵則：呈現層必須可抽換

整個架構**不能賭原生對話能成**。劇情引擎只透過抽象介面跟「對話呈現」溝通，backend 可換。

```
config/quests/*.json  (劇情定義)
        │  載入 + 驗證
┌───────▼────────────────────────────────┐
│  QuestEngine（C++ 狀態機）                 │  ← 不知道也不在乎對話長怎樣
│  變數 / 條件 / 動作 / 目標 / 轉移 / 觸發     │
└───────┬────────────────────────────────┘
        │  IDialoguePresenter::Present(node) → 回傳玩家選了哪個 choice index
        │
   ┌────┴──────────────┬─────────────────────┐
   ▼                   ▼                     ▼
MessageBoxPresenter  NativePresenter        ScaleformPresenter
（保底，必做）         （spike，不保證可行）    （phase 4，選配）
```

`IDialoguePresenter` 介面（概念，非最終簽名）：

```cpp
struct PresentedChoice { std::string text; bool enabled; int index; };
struct PresentedNode {
    std::string speaker;                 // 角色 id（給 presenter 解析顯示名/語音）
    std::vector<std::string> lines;      // NPC 台詞（可多句）
    std::vector<PresentedChoice> choices;// 已過濾條件後可見的選項
};
class IDialoguePresenter {
public:
    virtual ~IDialoguePresenter() = default;
    // 顯示一個節點，透過 callback 回傳玩家選的 choice.index（-1 = 取消/結束）
    virtual void Present(const PresentedNode& node, std::function<void(int)> onChoice) = 0;
    virtual void Close() = 0;
};
```

實作策略：**先把整條任務流程用 `MessageBoxPresenter` 打通**，原生對話當平行 spike，能成才接 `NativePresenter`。spike 失敗只是換 backend，狀態機 / JSON / 存檔全部不動。

---

## 2. JSON Schema（手寫 + LLM 的契約）

一個任務一個檔，放 `config/quests/<id>.json`，啟動時用 `SystemUtil::File::GetConfigs` 掃描載入。

### 2.1 Form 參照格式

沿用本專案 `FormUtil::Parse` 慣例，所有指向遊戲 form 的欄位接受兩種寫法：
- `"0x0001A6AC~Skyrim.esm"` — FormID + 來源 plugin（delimiter `~`）
- `"BelethorServices"` — EditorID（內部走 `LookupByEditorID`）

### 2.2 頂層結構

```json
{
  "id": "mq_merchant_secret",
  "title": "商人的秘密",
  "version": 1,
  "characters": { /* 角色綁定，見 2.3 */ },
  "vars": { "trust": 0, "knows_secret": false },
  "objectives": { /* 目標，見 2.4 */ },
  "dialogues": { /* 對話樹，見 2.5 */ },
  "triggers": [ /* 世界事件觸發，見 2.6 */ ],
  "on_start": [ /* 任務啟動時跑的 actions */ ]
}
```

- `id`：**全域唯一穩定字串**，co-save 的 key。一旦發佈不可改（改了等於新任務）。
- `vars`：任務內變數，型別由初值決定（number / bool / string），隨存檔持久。

### 2.3 角色綁定（casting registry）

JSON 用穩定別名（如 `"merchant"`）引用角色，執行期解析成實際 actor。兩種綁定:

```json
"characters": {
  "merchant": { "bind": "existing", "ref": "BelethorServices" },
  "thug":     { "bind": "spawn", "template": "0x00000007", "name": "可疑的打手" }
}
```

- `existing`：鎖定遊戲既有 NPC（身分穩定）。`ref` 是 Form 參照。
- `spawn`：用 `template`（模板 NPC）動態生（沿用 `NpcGenerator::SpawnNpc` 模式）。`name` 覆寫顯示名。
- 解析結果（FormID / spawned actor handle）寫入 co-save；讀檔後重新解析，spawned actor 若失效依政策重生或標記中斷。

### 2.4 目標（自製日誌追蹤，先不碰原生 QUST）

```json
"objectives": {
  "obj_meet":   { "text": "去白漫城找商人 Belethor", "state": "active" },
  "obj_decide": { "text": "決定要不要幫商人",        "state": "inactive" }
}
```

- `state`：`inactive` | `active` | `done` | `failed`。初值在此設定。
- 顯示先用通知 / 自製追蹤；要原生日誌再評估動態 `TESQuest`（見 §5 風險）。

### 2.5 對話樹

```json
"dialogues": {
  "talk_merchant": {
    "entry": "greet",
    "nodes": {
      "greet": {
        "speaker": "merchant",
        "lines": ["你不是本地人吧？"],
        "choices": [
          { "text": "誰在問？", "goto": "wary" },
          { "text": "[等級5] 只是路過。",
            "when": { "player_level_gte": 5 },
            "then": [ { "add_var": { "var": "trust", "value": 1 } } ],
            "goto": "lie" }
        ]
      },
      "wary": { "speaker": "merchant", "lines": ["哼。"], "end": true }
    }
  }
}
```

- `node`：`speaker` + `lines`（NPC 台詞）+ `choices` 或 `end: true`。
- `choice`：`text`（玩家選項）、選配 `when`（不滿足則隱藏/灰掉）、選配 `then`（選後跑的 actions）、`goto`（下一節點）或 `end: true`。
- 節點的 `saidOnce` 之類旗標由引擎持久化。

### 2.6 觸發（世界事件 → 動作）

```json
"triggers": [
  { "on": "dialogue_end", "dialogue": "talk_merchant",
    "do": [ { "complete_objective": "obj_meet" }, { "set_objective_active": "obj_decide" } ] },
  { "on": "actor_death", "character": "thug",
    "do": [ { "complete_quest": true } ] }
]
```

每個觸發 = `on`（事件型別）+ 過濾欄位 + 選配 `when` + `do`（actions）。

---

## 3. 封閉詞彙表（LLM 不能亂掰，validator 能擋）

### 3.1 條件 `when`（皆為布林，物件內多鍵預設 AND）

| key | 參數 | 意義 |
|-----|------|------|
| `var_eq` / `var_neq` | `{var, value}` | 任務變數等於 / 不等於 |
| `var_gte` / `var_lte` | `{var, value}` | 數值變數 ≥ / ≤ |
| `player_level_gte` / `_lte` | number | 玩家等級 |
| `player_gold_gte` | number | 玩家金幣 |
| `player_has_item` | `{form, count}` | 持有物品 ≥ count |
| `player_has_spell` | `{form}` | 已學法術 |
| `character_alive` / `character_dead` | `{character}` | 角色死活 |
| `objective_state` | `{objective, state}` | 目標處於某狀態 |
| `in_location` | `{location}` | 玩家在某地點 |
| `time_of_day` | `{from, to}` | 遊戲時間區間（時） |
| `random` | `{chance}` | 0.0–1.0 機率 |
| `all` / `any` | `[ ...conditions ]` | 邏輯組合 |
| `not` | `{ ...condition }` | 反向 |

### 3.2 動作 `then` / `do` / `on_start`（封閉動詞表）

| 動詞 | 參數 | 對應 API（已驗證/可用）|
|------|------|------|
| `set_var` / `add_var` | `{var, value}` | 引擎內部 |
| `give_item` / `remove_item` | `{form, count}` | `Actor::AddObjectToContainer` / `RemoveItem` |
| `give_gold` / `remove_gold` | `{amount}` | 同上（Gold001）|
| `add_spell` / `remove_spell` | `{form}` | `Actor::AddSpell`（見 NpcGenerator）|
| `add_shout` | `{form}` | `ActorEquipManager::EquipShout` |
| `spawn_character` | `{character}` | casting registry → `PlaceObjectAtMe` |
| `move_character` | `{character, to}` | `SetPosition`（to: `player`/marker/另一 character）|
| `teleport_player` | `{to}` | `MoveTo` / `ObjectUtil::Transform::TranslateTo` |
| `start_combat` | `{character, against}` | `Actor::StartCombat` |
| `set_relationship` | `{character, level}` | relationship rank |
| `play_idle` | `{character, idle}` | `AnimUtil::Idle::Play`（util.h 已有）|
| `add_map_marker` | `{pos, name}` | 動態 map marker |
| `set_objective_active` / `complete_objective` / `fail_objective` | `{objective}` 或字串 | 引擎內部 |
| `complete_quest` / `fail_quest` | `true` | 引擎內部 |
| `start_dialogue` | `{dialogue}` | 開一段對話樹 |
| `show_message` | `{text}` | `RE::DebugNotification` / HUD |
| `play_sound` | `{form}` | 音效 |

### 3.3 觸發 `on`（事件型別 → BSTEventSink）

| `on` | 過濾欄位 | 對應事件 |
|------|----------|----------|
| `quest_start` | — | 任務啟動 |
| `dialogue_end` | `{dialogue}` | 對話樹結束 |
| `activate` | `{character}` | 對 NPC 互動（談話入口）`TESActivateEvent` |
| `actor_death` | `{character}` | `TESDeathEvent` |
| `item_acquired` | `{form}` | `TESContainerChangedEvent` |
| `location_entered` | `{location}` | `TESCellAttachDetachEvent` / 位置檢查 |
| `objective_completed` | `{objective}` | 引擎內部 |

> 新增動詞/條件/觸發 = 改這三張表 + 改對應的 C++ 解析，**不要在 JSON 端發明新語法**。

---

## 4. 持久化（co-save）

用 `SKSE::SerializationInterface`（見 `COMMONLIBSSE_INDEX.md` §SKSE）。

- **key 一律用 JSON 穩定字串 ID**（任務 id / 目標 id / 節點 id / 角色別名），**絕不存動態 FormID 當主鍵** —— 動態 form 的 `0xFF...` ID 讀檔後會變。
- 每個進行中的任務存：當前對話節點、`vars` 值、各目標 `state`、角色綁定解析結果（FormID 或 spawned handle）、節點 saidOnce 旗標。
- 讀檔流程：先重新載入 JSON 定義（程式碼提供），再從 co-save 套用「玩家進度」。定義與進度分離 → 改 .json 文字（非結構）不會讓舊存檔壞掉。
- spawned 角色：存其 FormID，讀檔後驗證是否仍解析得到；失效則依政策重生或標記任務中斷。

---

## 5. 原生 DialogueMenu — spike 研究筆記

已讀過 `CommonLibSSE-NG/include/RE/` 相關 header，結論：**結構都摸得到，但沒有現成「開一段自訂對話」的入口，是 R&D，不是食譜。**

可用素材：
- `RE::MenuTopicManager`（singleton）member layout 全有：`dialogueList`（`BSSimpleList<Dialogue*>*`）、`speaker`、`topLevelBranches`、`rootTopicInfo`。
- `RE::DialogueItem` **有 public 建構子** `DialogueItem(quest, topic, topicInfo, speaker)`；`RE::DialogueResponse` 有 `text`（字幕）與 `voice`（語音檔名）欄位 —— 這是選單實際 render 的記憶體結構。
- `RE::TESTopic` / `RE::TESTopicInfo` 是 `TESForm` 子類（`FormType::Dialogue` / `Info`），有 `topicInfos`、`ownerQuest`、`objConditions`(`TESCondition`)。
- 線索旗標：`TOPIC_INFO_FLAGS::kForceSubtitle`、`kNoLIPFile` —— 讓「無語音也強制顯示字幕」可行的關鍵。

兩條候選路：
1. **動態造 form**：造 `TESTopic`/`TESTopicInfo` 掛到 quest，靠引擎撈出來。風險：引擎依 branch/subtype/saidOnce 過濾，未必撈到執行期塞的；持久化麻煩。
2. **記憶體/hook 注入**：用 `DialogueItem` public ctor 建好，塞進 `MenuTopicManager::dialogueList`，hook「topic 被選」callback 驅動狀態機。較可控但要 trampoline hook + 逆 populate 位址。

**已知硬傷：無語音**。`DialogueResponse` 沒語音檔時引擎通常直接跳台詞。緩解方向：`kForceSubtitle` + 一個極短的靜音 .fuz。

**spike go/no-go 準則**：能否在執行期對一個任意 NPC 開出一段「顯示自訂字幕文字 + 至少 2 個可點選項 + 點選後能回呼到 C++」的對話。做不到就停在 MessageBox backend。

---

## 6. 預定原始碼分層（實作時才建，記得登錄 cmake）

> 本專案 `CMakeLists.txt` 不 glob：新 `.cpp` 登 `cmake/sourcelist.cmake`、新 `.h` 登 `cmake/headerlist.cmake`，否則不編譯。

```
src/quest/QuestEngine.{h,cpp}      載入 JSON、持有狀態機、總分派
src/quest/QuestState.h             執行期狀態結構（vars/objectives/節點）
src/quest/Conditions.{h,cpp}       §3.1 條件求值
src/quest/Actions.{h,cpp}          §3.2 動作執行
src/quest/Triggers.{h,cpp}         §3.3 事件 sink → 觸發分派
src/quest/CastingRegistry.{h,cpp}  §2.3 角色綁定/解析
src/quest/Persistence.{h,cpp}      §4 co-save 序列化
src/dialogue/IDialoguePresenter.h  §1 抽象呈現介面
src/dialogue/MessageBoxPresenter.{h,cpp}   保底 backend
src/dialogue/NativePresenter.{h,cpp}       §5 spike backend
config/quests/*.json               劇情定義
config/schema/quest.schema.json    JSON Schema（驗證 + 餵 LLM）
```

---

## 7. 分期路線

- **Phase 0 — 骨架**：`IDialoguePresenter` + `MessageBoxPresenter` + 最小狀態機 + 1 個寫死的 JSON 對話樹跑通分支。觸發沿用「對目標施法 = 開始對話」。
- **Phase 1 — 核心**：完整條件/動作詞彙 + co-save 持久化 + casting registry + 目標狀態。
- **Phase 2 — 任務化**：世界事件觸發（殺/到/拿）+ 自製日誌追蹤 + 多任務並行 + JSON Schema validator（嚴格錯誤訊息）。
- **Phase 3 — 原生對話 spike**：依 §5 go/no-go；成功才接 `NativePresenter`。
- **Phase 4（選配）**：自製 Scaleform 對話 UI。

---

## 8. 待定問題（之後決策）

- 多任務同時進行的優先序 / 互斥規則。
- 對話進行中存檔的行為（鎖存檔？還是允許並在讀檔後重開對話？）。
- spawned 角色讀檔失效的預設政策（重生 / 中斷 / 視任務標記）。
- 是否要 MCM 風格的任務日誌 UI（vs 純通知）。
- LLM 生成後的自動驗證流程（離線 validator vs 載入時驗證）。
