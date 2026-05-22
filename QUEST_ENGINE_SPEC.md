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

---

## 3. 狀態機語意（跨實作 MUST 一致）

### 3.1 任務生命週期
1. 載入 + 驗證（§7）→ 套用持久化進度（§6）；無進度則為初始狀態（`vars` 初值、`objectives` 初始 state）。
2. 任務啟動時執行 `on_start`，並發出 `quest_start` 事件。
3. 任務在 `complete_quest` / `fail_quest` 後進入終止狀態，MUST 停止其觸發回應。

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

> id 類核心動作收「字串 id 簡寫」（如 `{"complete_objective": "obj_meet"}`），對手寫與 LLM 生成較友善。

### 4.3 核心觸發（無遊戲依賴）

| `on` | 過濾 | 來源 |
|------|------|------|
| `quest_start` | — | 任務啟動 |
| `dialogue_end` | `{dialogue}` | 對話結束 |
| `objective_completed` | `{objective}` | 目標完成 |

### 4.4 Adapter 擴充機制

- adapter MUST 向核心**宣告**它支援的擴充條件/動作/觸發，每項附其參數 schema。
- 「**有效 schema**」= 核心詞彙 schema ∪ adapter 宣告的擴充。驗證（§7）對有效 schema 進行。
- 核心遇到擴充詞彙時，MUST 把（動詞名、參數、已解析實體）轉交對應能力埠（§5）；核心**不理解**擴充詞彙的內部語意。
- 範例擴充（非規格強制，供參考）：`give_item`、`spawn_character`、`start_combat`、`teleport_player`；條件 `player_level_gte`、`player_has_item`、`character_alive`；觸發 `activate`、`actor_death`、`item_acquired`、`location_entered`。各遊戲支援哪些由 adapter 決定。

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

### 5.7 選配埠
- Clock（遊戲時間）、Logger、RNG。RNG 見 §8。

---

## 6. 持久化模型

- 核心 MUST 能把所有進行中任務的**進度**序列化成版本化 blob，並還原。
- 進度包含：每個任務的 `vars` 值、各 objective state、當前對話節點（若進行中）、節點 saidOnce 之類旗標、實體綁定的可序列化識別。
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

MAY：宣告任意 adapter 擴充詞彙；提供多個 DialoguePresenter；提供離線 validator。

---

## 8. 確定性與邊界情況（跨實作 MUST 對齊）

- **動作執行順序**：動作串 MUST 依陣列順序、同步、依序執行。
- **動作失敗**：擴充動作失敗 MUST 記錄並繼續執行同串後續動作（不中止整串）。流程性核心動作（`start_dialogue` / `complete_quest`…）的失敗處理見各自定義。
- **未定義參照**：`goto` 指向不存在節點、條件/動作引用未宣告變數或未知 alias → **驗證期 MUST 報錯**；若執行期才發現，MUST 記錄並安全中止當前流程（不崩潰）。
- **型別不符**：`set_var` / 比較遇到型別不符 → 驗證期 MUST 報錯。
- **`random`（確定性，由存檔種子導出）**：MUST 由「`master_seed`（§6）+ 該 `random` 的穩定 site key」確定性導出，使**重載同一存檔結果不變**。
  - site key = `任務 id` + 該 `random` 在 JSON 中的穩定路徑（如 `dialogues.greet.choices[1].when.random`），或作者明設的 `key` 欄位。
  - 導出 = `PRF(master_seed, quest_id, site_key)` 映到 `[0,1)`，與 `chance` 比較。`PRF` 的具體演算法 MUST 在 SPEC 附錄釘死（待補），讓不同語言實作得出**同一結果**。
  - 性質：(a) 重載穩定；(b) 不同 site 互相獨立；(c) **同一 site 重複求值結果相同（冪等）**。
  - 要「每次命中可能不同」的擲骰：MUST 改用顯式變數（例如 `add_var` 計數並把計數納入 site key），核心**不提供**非冪等 `random`。
- **多任務並行（暫定，可能改）**：每個任務 MAY 宣告 `priority` ∈ {`high`,`normal`,`low`}（預設 `normal`，意圖對應主線/支線/雜項）。多任務爭用互斥資源（同時要開對話、或控制同一角色）時：**較高 priority 優先；同級則先搶先贏**（已持有/先啟動者保留）。強制執行範圍暫不深入定義，實作初期 MAY 僅記錄欄位而不強制。
- **重複事件**：同一事件對到多觸發時依陣列順序執行（§3.3）。

---

## 9. 未定事項（Open Issues）

下列尚未釘死，實作前需先定案。彙整自全文行內標記。

1. **`PRF` 演算法（高優先，阻擋 `random` 實作）**：`random` 確定性導出用的 `PRF(master_seed, quest_id, site_key)`（§8）尚未定案。跨語言實作 MUST 得出**同一結果**，故須指定一個各語言都易重現的明確演算法。候選：對 `master_seed | quest_id | site_key` 做 SHA-256（或 SipHash），取前 64 bit 當整數除以 2^64 映到 `[0,1)`。**未定案前 `random` 不應實作。** 定案後寫入附錄 B。
2. **priority 強制範圍（暫定，§8）**：多任務 `priority` 的「互斥資源」具體涵蓋哪些（對話、角色控制、其他？）、仲裁時機，暫定「高優先先、同級先搶先贏」，細節待實作期定。
3. **`site key` 的精確規則（§8）**：`random` 的穩定路徑表示法（陣列索引格式、作者 `key` 與自動路徑的優先序）需與附錄 B 一併釘死，否則不同實作算出的 site key 不一致。

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

**這個練習對目前 SPEC 的檢驗結論（多為「驗證通過」）：**

1. **`characters` 綁定描述是 per-adapter 的，所以一份劇情的 `characters` 區塊本來就不跨 adapter** —— 這與「引擎可攜、內容各遊戲自己寫」的選擇一致。邊界畫對了。
2. **缺某詞彙不會崩，會在驗證期被擋**：CLI 不宣告 `time_of_day`，於是用到它的劇情無法通過 CLI 的「有效 schema」。這正面驗證了 §4.4「adapter 宣告能力 + 有效 schema」的設計。
3. **`show_message` 走呈現埠** 而非直接碰遊戲 → CLI 用 print 即可，無洩漏。
4. 若 CLI 實作不出某個**核心**詞彙（§4.1–4.3）或狀態機語意（§3），那就是核心抽得不夠乾淨，要回頭修 SPEC —— 這是這個 harness 最大的價值。

> 註：第一個 adapter（Skyrim）+ 這個 CLI harness 並行，是驗證「核心零遊戲依賴」最便宜的方式。實作期 `src/core/` 應能同時被 Skyrim adapter 與一個小型 CLI 測試程式連結。
