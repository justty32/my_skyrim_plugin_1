# 劇情/對話引擎 — 可攜規格（Quest Engine Spec）

> 狀態：**設計階段**。本檔是**語言/引擎中立的規格**，是這個專案真正可攜的產物。
> 任何語言、任何引擎（C++/SKSE、C#/Unity、GDScript/Godot、JS/網頁…）只要照本規格實作一個 runtime + adapter，就能跑同一套劇情 JSON。
> C++/SKSE 的參考實作與 Skyrim adapter 細節見 `QUEST_ENGINE_DESIGN.md`。

本文用 RFC 風格的 **MUST / SHOULD / MAY** 表達一致性要求。

---

## 0. 三層架構

```
┌──────────────────────────────────────────────────┐
│  劇情 JSON（內容，各遊戲自己寫，綁該遊戲的角色/物品）       │
└───────────────────────┬──────────────────────────┘
                        │ 依本規格解讀
┌───────────────────────▼──────────────────────────┐
│  可攜核心 Runtime（本規格定義其行為，零遊戲依賴）          │
│  狀態機 / 條件求值 / 動作分派 / 對話流程 / 持久化格式        │
└───────────────────────┬──────────────────────────┘
                        │ 透過「能力埠 capability ports」呼叫遊戲（§5）
┌───────────────────────▼──────────────────────────┐
│  遊戲 Adapter（每個遊戲/引擎各寫一個，本規格只定義其契約）    │
└──────────────────────────────────────────────────┘
```

可攜的是**核心 Runtime 的「行為規格」**與**JSON 格式**。不可攜的是劇情內容（綁定具體遊戲）與 adapter。

---

## 1. 概念模型

| 概念 | 說明 |
|------|------|
| **Quest（任務）** | 一個 JSON 文件。有穩定 `id`、變數、目標、對話樹、觸發。 |
| **Variable（變數）** | 任務範圍的具名值（number / bool / string）。隨進度持久化。 |
| **Objective（目標）** | 具狀態（inactive/active/done/failed）的待辦項，供追蹤顯示。 |
| **Dialogue（對話樹）** | 具名的節點圖。節點有台詞與選項，選項可帶條件、動作、跳轉。 |
| **Trigger（觸發）** | 「事件 + 過濾 + 條件 → 動作串」。事件來自核心或遊戲 adapter。 |
| **Condition（條件）** | 布林判斷式。核心詞彙 + adapter 擴充（§4）。 |
| **Action（動作）** | 副作用指令。核心詞彙 + adapter 擴充（§4）。 |
| **Entity ref（實體參照）** | **對核心不透明的字串**，由 adapter 解析成遊戲實體（§5.1）。 |
| **Global var（全域變數）** | 跨任務、系統層持久的具名值。以 `global.<name>` 在任意任務的條件/動作中參照（§2.4）。 |
| **Timer（計時器）** | 由 `schedule` 動作排定的未來事件；到期時由 Clock 埠驅動 `timer` 事件（§4.2/§4.3）。 |

---

## 2. JSON 文件格式

一個任務一個檔。頂層欄位：

```json
{
  "id": "merchant_secret",
  "title": "商人的秘密",
  "version": 1,
  "priority": "normal",
  "characters": { "<alias>": { /* 綁定描述，adapter 定義，§5.1 */ } },
  "vars": { "<name>": <number|bool|string> },
  "objectives": { "<id>": { "text": "...", "state": "inactive|active|done|failed" } },
  "dialogues": { "<id>": { "entry": "<nodeId>", "nodes": { "<nodeId>": <Node> } } },
  "triggers": [ <Trigger> ],
  "on_start": [ <Action> ]
}
```

- `id` MUST 全域唯一且穩定，是持久化主鍵（§6）。發佈後改 `id` 視為新任務。
- `version` 是內容版本，供 adapter 在格式遷移時判斷。
- `priority` 選配，`high` / `normal` / `low`（預設 `normal`），多任務爭用互斥資源時的優先序（§8，**暫定可能改**）。
- 變數型別由初值決定，執行期 MUST NOT 改變型別。

### 2.1 Node（對話節點）

```json
{
  "speaker": "<characterAlias>",
  "lines": ["NPC 台詞，可多句"],
  "choices": [ <Choice> ],        // 與 "end" 二擇一
  "end": true
}
```

### 2.2 Choice（玩家選項）

```json
{
  "text": "玩家選項文字",
  "when": <Condition>,            // 選配；不滿足時隱藏或停用（§3.4）
  "then": [ <Action> ],           // 選配；選後依序執行
  "goto": "<nodeId>",             // 與 "end" 二擇一
  "end": true
}
```

### 2.3 Trigger（觸發）

```json
{
  "on": "<eventType>",
  "<filterKey>": "<value>",       // 視事件型別而定（如 dialogue/character）
  "when": <Condition>,            // 選配，額外守衛
  "do": [ <Action> ]
}
```

### 2.4 全域變數（globals）

跨任務共享、隨存檔在系統層持久的具名值（型別由初值定，比照任務 `vars`）。

- 在任意任務的條件/動作中，以 **`global.<name>`** 前綴的 `var` 參數參照（如 `{"add_var":{"var":"global.whiterun_tasks_done","value":1}}`）。沿用既有 `set_var`/`add_var`/`var_*`，**不新增動詞**。
- 全域變數 MUST 在**系統層預先宣告**（名稱 + 初值，型別由初值定）；宣告來源/檔案格式由實作決定（如一份 manifest 文件）。未宣告即被參照 MUST 於驗證期報錯（§7）。
- 在系統層持久化，**不綁任何單一任務**（§6）；任務 `reset_quest`（§3.1）不影響全域變數。

---

## 3. 狀態機語意（跨實作 MUST 一致）

### 3.1 任務生命週期
1. 載入 + 驗證（§7）→ 套用持久化進度（§6）；無進度則為初始狀態（`vars` 初值、`objectives` 初始 state）。
2. 任務啟動時執行 `on_start`，並發出 `quest_start` 事件。
3. 任務在 `complete_quest` / `fail_quest` 後進入終止狀態，MUST 停止其觸發回應。
4. `reset_quest`（§4.2）把當前任務復位到初始狀態：`vars`/`objectives` 回初值、清當前對話與 saidOnce 旗標、清本任務待發計時器，再執行 `on_start` 並重發 `quest_start`。**全域變數（§2.4）不受影響**——跨循環的計數應放全域變數。供可重複任務（如反覆召喚）。

### 3.2 對話流程
1. `start_dialogue` 進入指定對話樹的 `entry` 節點。
2. 呈現節點：把 `speaker`、`lines`、以及**通過 `when` 過濾後**的 `choices` 交給對話呈現埠（§5.5）。
3. 玩家選擇後：依序執行該 choice 的 `then`，再依 `goto` 進入下一節點，或 `end` 結束對話。
4. 對話結束時發出 `dialogue_end` 事件（附對話樹 id）。
5. 一次 MUST 至多一段對話進行中。

### 3.3 觸發求值
- 事件發生時，所有 `on` 相符且過濾鍵相符、且 `when`（若有）為真的觸發，其 `do` MUST 被執行。
- 同一事件對到多個觸發時的執行順序 = JSON 陣列順序。

### 3.4 條件失敗的選項
- choice 的 `when` 為假時，實作 MUST 不讓玩家選到它。SHOULD 提供「隱藏」或「停用(灰)」兩種呈現，由 adapter/呈現埠決定，預設隱藏。

---

## 4. 詞彙契約

詞彙分兩層：**核心詞彙**（每個實作 MUST 支援且語意一致）與 **adapter 擴充詞彙**（各遊戲宣告）。

### 4.1 核心條件（無遊戲依賴）

| key | 參數 | 語意 |
|-----|------|------|
| `var_eq` / `var_neq` | `{var, value}` | 變數等於 / 不等於（同型別比較）|
| `var_gte` / `var_lte` | `{var, value}` | number 變數 ≥ / ≤ |
| `objective_state` | `{objective, state}` | 目標處於指定 state |
| `random` | `{chance}` | 0.0–1.0 機率為真（§8 確定性）|
| `all` / `any` | `[ <Condition>… ]` | 邏輯 AND / OR |
| `not` | `<Condition>` | 邏輯反向 |

> 條件物件含多個 key 時 MUST 視為 AND。

### 4.2 核心動作（無遊戲依賴）

| 動詞 | 參數 | 語意 |
|------|------|------|
| `set_var` | `{var, value}` | 設變數（型別 MUST 相符）|
| `add_var` | `{var, value}` | number 變數加值 |
| `set_objective_active` | 字串（objective id）| 目標轉 active |
| `complete_objective` / `fail_objective` | 字串（objective id）| 目標轉 done / failed，發出 `objective_completed`（done 時）|
| `complete_quest` / `fail_quest` | `true` | 任務終止 |
| `start_dialogue` | 字串（dialogue id）| 開一段對話樹 |
| `show_message` | 字串 或 `{text}` | 經呈現埠顯示短訊息（§5.5）|
| `schedule` | `{after_hours\|at, key}` | 排定計時器：經 Clock 埠在 `after_hours`（遊戲內小時）後、或絕對遊戲時間 `at`，發出 `timer` 事件（filter=`key`）。重排同 `key` MUST 取代舊到期時間（§6/§8）。需 Clock 埠（§5.7）|
| `reset_quest` | `true` | 復位當前任務（語意見 §3.1），供可重複任務。**不動全域變數** |

> id 類核心動作收「字串 id 簡寫」（如 `{"complete_objective": "obj_meet"}`），對手寫與 LLM 生成較友善。
>
> `set_var`/`add_var` 與 §4.1 的 `var_*` 條件，其 `var` MAY 用 `global.<name>` 指向全域變數（§2.4）。

### 4.3 核心觸發（無遊戲依賴）

| `on` | 過濾 | 來源 |
|------|------|------|
| `quest_start` | — | 任務啟動 |
| `dialogue_end` | `{dialogue}` | 對話結束 |
| `objective_completed` | `{objective}` | 目標完成 |
| `timer` | `{key}` | `schedule`（§4.2）排定的計時器到期 |

### 4.4 Adapter 擴充機制

- adapter MUST 向核心**宣告**它支援的擴充條件/動作/觸發，每項附其參數 schema。
- 「**有效 schema**」= 核心詞彙 schema ∪ adapter 宣告的擴充。驗證（§7）對有效 schema 進行。
- 核心遇到擴充詞彙時，MUST 把（動詞名、參數、已解析實體）轉交對應能力埠（§5）；核心**不理解**擴充詞彙的內部語意。
- 範例擴充（非規格強制，供參考）：`give_item`、`spawn_character`、`start_combat`、`teleport_player`；條件 `player_level_gte`、`player_has_item`、`character_alive`；觸發 `activate`、`actor_death`、`item_acquired`、`location_entered`。各遊戲支援哪些由 adapter 決定。

### 4.5 標準可攜擴充：非同步訊息（deliver_message / message_ack）

「延後送達、由玩家確認後才推進」的訊息（信件、信使、收件匣）有與遊戲無關的核心，但非每個 adapter 都需要，故定為**標準可攜擴充**（SHOULD，非核心 MUST）：adapter 若支援，MUST 用本節釘死的 schema，使內容在支援它的 adapter 間可攜；不支援的 adapter，用到它的內容於驗證期被擋（§4.4）。

- 動作 `deliver_message` `{ to:"player", subject, body, key }`：把一則訊息排入送達。送達**渲染**走呈現層 / ActionRunner（adapter 定，如 Skyrim 信使+note、CLI 印出待 ack），與引擎解耦。
- 觸發 `message_ack` `{ key }`：玩家**確認/讀取**該訊息時發出，由 EventSource 回送核心。

> 與即時的 `show_message`（§4.2）區別：`show_message` 即時、單向、不回事件；`deliver_message` 延後、需確認、確認後以 `message_ack` 推進。兩者語意不可互換。
> 未確認前重載存檔：未確認訊息 MUST 隨進度持久化（§6），讀檔後仍待確認。

---

## 5. 能力埠（capability ports）

核心透過下列抽象「埠」與遊戲溝通。本規格定義**職責**，不定義語言簽名——各實作用自己的慣用法（C++ 介面、C# interface、GDScript、JS callback…）。

### 5.1 EntityResolver（實體解析）
- 把 `characters` 的綁定描述、以及動作/條件參數中的實體參照字串，解析成遊戲實體控制代碼。
- 綁定描述格式由 adapter 定義（如 Skyrim 用 `{bind:"existing", ref:"EditorID"}` / `{bind:"spawn", template, name}`）。
- 解析結果 MUST 可序列化成穩定識別，供持久化（§6）。

### 5.2 ActionRunner（執行擴充動作）
- 給定（動詞、參數、已解析實體），執行其遊戲副作用。
- SHOULD 回報成功/失敗；失敗時核心 MUST 記錄並**繼續**執行同串後續動作（§8）。

### 5.3 ConditionEvaluator（求值擴充條件）
- 給定（條件 key、參數、已解析實體），回傳布林。
- 無法求值（如實體失效）SHOULD 回傳 false 而非中止。

### 5.4 EventSource（事件來源）
- 讓核心訂閱擴充事件型別；事件發生時把（事件型別、相關實體、payload）送回核心驅動觸發（§3.3）。

### 5.5 DialoguePresenter（對話呈現）
- 顯示一個已過濾節點（speaker / lines / choices），回傳玩家選的 choice 索引（或「取消」）。
- 顯示 `show_message`。
- **此埠刻意與引擎解耦**：同一遊戲可有多個實作（如 Skyrim 的 MessageBox / 原生對話選單 / 自訂 UI），互換 MUST NOT 影響核心。

### 5.6 PersistenceBackend（持久化後端）
- 把核心給的不透明進度 blob 寫進當前存檔、讀回。鍵結到「當前遊戲存檔」的方式由 adapter 決定（如 SKSE co-save）。

### 5.7 Clock / Logger / RNG 埠
- **Clock（遊戲時間）**：原為選配，但 `schedule`/`timer`（§4.2/§4.3）一旦使用即**必需**——核心靠它判斷計時器到期。不支援 Clock 的實作 MUST NOT 宣稱支援含 `schedule` 的內容。
- Logger、RNG 為選配。RNG 見 §8。

---

## 6. 持久化模型

- 核心 MUST 能把所有進行中任務的**進度**序列化成版本化 blob，並還原。
- 進度包含：每個任務的 `vars` 值、各 objective state、當前對話節點（若進行中）、節點 saidOnce 之類旗標、實體綁定的可序列化識別。
- 進度 blob MUST 含**全域變數**（§2.4）當前值，鍵用全域變數名，於**系統層**（不綁單一任務）序列化。
- 進度 blob MUST 含**待發計時器**（§4.2）：每筆為（任務 id、`key`、絕對到期遊戲時間）。讀檔後到期時間已過者 MUST 視為到期（立即/補發 `timer`），未到者續排（§8）。
- 進度 blob MUST 含**未確認的非同步訊息**（§4.5，若 adapter 支援）：讀檔後仍待 `message_ack`。
- 進度 blob MUST 含一個**主種子 `master_seed`**：於該存檔首次初始化任務系統時產生一次、之後固定不變，供 `random` 確定性導出（§8）。
- **所有鍵 MUST 用 JSON 的穩定字串 ID**（任務 id / objective id / 節點 id / 角色 alias）。**MUST NOT** 用遊戲執行期動態 ID 當主鍵（那種 ID 跨存檔不穩）。
- **定義與進度分離**：還原時先由程式重新載入 JSON 定義，再套用 blob 的進度。→ 改 JSON 的「文字內容」（非結構）SHOULD NOT 弄壞舊存檔。
- blob 內含 schema/格式版本，供遷移。

---

## 7. 驗證與一致性（conformance）

一個合規實作 MUST：
1. 對**有效 schema**（§4.4）驗證每個任務檔；驗證失敗 SHOULD 指出檔名 + 路徑（哪個節點/選項/動作）。
2. 完整支援核心詞彙（§4.1–4.3）且語意與本規格一致。
3. 依本規格的狀態機語意（§3）運作。
4. 依 §6 持久化，鍵用穩定字串 ID。
5. 把擴充詞彙轉交能力埠，不自行臆測其語意。
6. `global.<name>` 參照 MUST 指向已宣告全域變數（§2.4），否則驗證期報錯。

MAY：宣告任意 adapter 擴充詞彙；提供多個 DialoguePresenter；提供離線 validator。

---

## 8. 確定性與邊界情況（跨實作 MUST 對齊）

- **動作執行順序**：動作串 MUST 依陣列順序、同步、依序執行。
- **動作失敗**：擴充動作失敗 MUST 記錄並繼續執行同串後續動作（不中止整串）。流程性核心動作（`start_dialogue` / `complete_quest`…）的失敗處理見各自定義。
- **未定義參照**：`goto` 指向不存在節點、條件/動作引用未宣告變數或未知 alias → **驗證期 MUST 報錯**；若執行期才發現，MUST 記錄並安全中止當前流程（不崩潰）。
- **型別不符**：`set_var` / 比較遇到型別不符 → 驗證期 MUST 報錯。
- **`random`（確定性，由存檔種子導出）**：MUST 由「`master_seed`（§6）+ 該 `random` 的穩定 site key」依**附錄 B** 導出，使**重載同一存檔結果不變**。
  - site key 由作者非空 `key` 或該條件的 RFC 6901 JSON Pointer 導出；精確字串規則見附錄 B。`quest_id` 是 PRF 的獨立欄位，不重複塞進 site key。
  - 導出 = `PRF(master_seed, quest_id, site_key)` 映到 `[0,1)`，以嚴格小於（`sample < chance`）比較；`chance=0` 永假、`chance=1` 永真。
  - 性質：(a) 重載穩定；(b) 不同 site 互相獨立；(c) **同一 site 重複求值結果相同（冪等）**。
  - 要「每次命中可能不同」的擲骰：MUST 改用顯式變數（例如 `add_var` 計數並把計數納入 site key），核心**不提供**非冪等 `random`。
- **多任務並行（暫定，可能改）**：每個任務 MAY 宣告 `priority` ∈ {`high`,`normal`,`low`}（預設 `normal`，意圖對應主線/支線/雜項）。多任務爭用互斥資源（同時要開對話、或控制同一角色）時：**較高 priority 優先；同級則先搶先贏**（已持有/先啟動者保留）。強制執行範圍暫不深入定義，實作初期 MAY 僅記錄欄位而不強制。
- **重複事件**：同一事件對到多觸發時依陣列順序執行（§3.3）。
- **計時器（確定性）**：`schedule` 到期時間 MUST 以**絕對遊戲時間**存 blob（§6），重載不變。需「隨機間隔」MUST 用 `random` + `add_var` 顯式導出（符合本節 `random` 冪等規則），核心不提供非冪等抖動。重排同 `key` 取代舊到期。
- **`reset_quest`**：復位後重跑 `on_start` MUST 確定性；全域變數不受復位影響，故跨循環計數以全域變數承載而非任務 `vars`。

---

## 9. 未定事項（Open Issues）

下列尚未釘死，實作前需先定案。彙整自全文行內標記。

1. ~~**PRF 演算法 / site key 精確規則**~~ → 已定案：SHA-256 + canonical binary framing + RFC 6901 path，見附錄 B。
2. **priority 強制範圍（暫定，§8）**：多任務 `priority` 的「互斥資源」具體涵蓋哪些（對話、角色控制、其他？）、仲裁時機，暫定「高優先先、同級先搶先贏」，細節待實作期定。

> 已於本版納入（原列 `COURT_WIZARD_DESIGN.md` §6 開放問題）：可重複任務 → `reset_quest`（§3.1/§4.2，搭配全域變數承載跨循環計數）；時間排程 → `schedule`/`timer`（§4.2/§4.3）；跨任務狀態 → 全域變數（§2.4）；非同步訊息 → 標準擴充（§4.5）。

> 內容類（非規格）的未定項（對話中存檔行為、spawned 失效政策、日誌 UI 等）記在 `QUEST_ENGINE_DESIGN.md` §8。

---

## 附錄 A：Headless CLI 參考 adapter（可攜性試金石 + 一致性 harness）

為了壓測本規格「真的跨引擎可攜」，定義一個**刻意與 Skyrim 毫無共通點**的假想 adapter：一個純文字、無遊戲引擎的命令列 runtime。它與 Skyrim 共享零假設（無 FormID、無 co-save、無 MessageBox），所以任何「偷渡進核心的 Skyrim 假設」都會在這裡露餡。它同時可當**離線 validator + 一致性測試 harness**。

各能力埠在 CLI 下的對應：

| 能力埠 | CLI 實作 |
|--------|----------|
| EntityResolver | 解析到記憶體物件；`characters` 綁定描述用 CLI 自己的格式（如 `{bind:"existing", ref:"<objName>"}` / `{bind:"spawn", template:"<typeId>"}`）|
| ActionRunner | 改記憶體狀態 / 印文字 |
| ConditionEvaluator | 查記憶體狀態 |
| EventSource | 由腳本檔或互動指令**模擬**事件（`actor_death` 等）|
| DialoguePresenter | 印 speaker/lines、列出 choices、讀 stdin 取索引 |
| PersistenceBackend | 把進度 blob 寫成一個 `.json` 檔；「當前存檔」= 一個 slot 檔名 |
| Clock / RNG / Logger | 系統時鐘 / 以 `master_seed` 實作 §8 的 PRF / 印 log |
| Timers（`schedule`/`timer`）| Clock + 互動 `advance-time` 指令推進遊戲時間，到期發 `timer` |
| Globals（§2.4）| 記憶體 map，slot 檔持久 |
| deliver_message / message_ack（§4.5）| 印出訊息、待互動 `ack <key>` 指令 |

**這個練習對目前 SPEC 的檢驗結論（多為「驗證通過」）：**

1. **`characters` 綁定描述是 per-adapter 的，所以一份劇情的 `characters` 區塊本來就不跨 adapter** —— 這與「引擎可攜、內容各遊戲自己寫」的選擇一致。邊界畫對了。
2. **缺某詞彙不會崩，會在驗證期被擋**：CLI 不宣告 `time_of_day`，於是用到它的劇情無法通過 CLI 的「有效 schema」。這正面驗證了 §4.4「adapter 宣告能力 + 有效 schema」的設計。
3. **`show_message` 走呈現埠** 而非直接碰遊戲 → CLI 用 print 即可，無洩漏。
4. 若 CLI 實作不出某個**核心**詞彙（§4.1–4.3）或狀態機語意（§3），那就是核心抽得不夠乾淨，要回頭修 SPEC —— 這是這個 harness 最大的價值。

> 註：第一個 adapter（Skyrim）+ 這個 CLI harness 並行，是驗證「核心零遊戲依賴」最便宜的方式。實作期 `src/core/` 應能同時被 Skyrim adapter 與一個小型 CLI 測試程式連結。

---

## 附錄 B：`random` 的跨語言 PRF（規範性）

本附錄釘死 `PRF(master_seed, quest_id, site_key)`。實作 MUST NOT 使用平台 `hash()`、PRNG state、locale、原生整數位元序或未規範的字串串接。

### B.1 輸入字串與 site key

- `master_seed` MUST 是存檔系統層持久化的**非空字串**。建議 host 首次建立存檔狀態時產生至少 128 bits 熵並存為小寫十六進位字串；PRF 把字串本身編碼，不解碼 hex。
- `quest_id` = 任務頂層 `id` 的原字串。
- 若 `random` 有作者明設的非空字串 `key`，`site_key = "key:" + key`。
- 否則 `site_key = "path:" + pointer`；`pointer` 是從任務文件根開始、指到該 `random` key 的 RFC 6901 JSON Pointer。例如 `dialogues.greet.nodes.entry.choices[1].when.random` 對應 `/dialogues/greet/nodes/entry/choices/1/when/random`。物件 key 中 `~` 編成 `~0`、`/` 編成 `~1`；陣列索引用無前導零十進位。
- `key:` / `path:` 是不同命名空間；相同明設 `key` 可讓不同位置刻意共用一次 deterministic roll。
- 三個字串都 MUST 取其 JSON 字串值的 UTF-8 bytes（RFC 3629），不含 BOM、不做 Unicode normalization、不做大小寫或 locale 轉換。實作遇到無法編成合法 UTF-8 的字串 MUST 驗證失敗。

### B.2 Canonical binary framing

先寫入固定 16 bytes ASCII domain tag：

```text
51 45 2d 52 41 4e 44 4f 4d 2d 50 52 46 2d 56 31
 Q  E  -  R  A  N  D  O  M  -  P  R  F  -  V  1
```

接著依序寫入 `master_seed`、`quest_id`、`site_key`。每欄格式皆為 `u64be(byte_length) || utf8_bytes`：長度是**該欄 UTF-8 byte 數**，以 unsigned 64-bit big-endian 固定 8 bytes 表示。完整輸入為：

```text
ASCII("QE-RANDOM-PRF-V1")
|| U64BE(len(UTF8(master_seed))) || UTF8(master_seed)
|| U64BE(len(UTF8(quest_id)))    || UTF8(quest_id)
|| U64BE(len(UTF8(site_key)))    || UTF8(site_key)
```

PRF digest = SHA-256(canonical input bytes)。SHA-256 指 FIPS 180-4，不得替換成平台 hash。

### B.3 Digest 到 `[0,1)` 與 chance

1. 取 digest 的前 8 bytes `digest[0..7]`，以 big-endian 解成 unsigned `uint64`：`word = Σ digest[i] × 2^(56-8i)`。
2. `top53 = word >> 11`。
3. `sample = top53 / 2^53`，以 IEEE-754 binary64 計算。因 numerator 小於 `2^53`，結果可精確表示且 MUST 落在 `[0,1)`，永不等於 `1.0`。
4. `random` 為真 iff `sample < chance`。`chance` MUST 是有限 number 且在 `[0,1]`；否則驗證失敗，若執行期才遇到則安全回傳 false。

低 11 bits 刻意捨棄，使 JavaScript、C++、C#、GDScript 等使用 binary64 的實作得到完全相同且不會向上捨入成 `1.0` 的值。

### B.4 Golden vectors

| # | `master_seed` | `quest_id` | `site_key` | canonical bytes (hex) | SHA-256 digest | first uint64 | `sample` |
|---|---|---|---|---|---|---|---|
| 1 | `seed` | `quest` | `path:/triggers/0/when/random` | `51452d52414e444f4d2d5052462d563100000000000000047365656400000000000000057175657374000000000000001c706174683a2f74726967676572732f302f7768656e2f72616e646f6d` | `0fe72c7122d71c586f354eb4be5f23787bc3c4b75e9e722b4144592e282ae7e8` | `0x0fe72c7122d71c58` | `0.062121179219357336` |
| 2 | `種子-01` | `任務/alpha` | `key:門檻` | `51452d52414e444f4d2d5052462d56310000000000000009e7a8aee5ad902d3031000000000000000ce4bbbbe58b992f616c706861000000000000000a6b65793ae99680e6aabb` | `0569538156f6057cb02ffddb1f895fdec269053b09ca57ba80fa7a71c5d404c4` | `0x0569538156f6057c` | `0.0211384001513224` |

Vector 1 的門檻結果：`chance=0` → false；`chance=sample` → false（嚴格 `<`）；下一個大於 sample 的 binary64 值 → true；`chance=1` → true。
