# 宮廷大法師 — Mod 設計（建於 Quest Engine 之上）

> 狀態：**設計階段，尚未實作**。
> 本檔是這個 mod 的**內容/系統設計**，建在 `QUEST_ENGINE_SPEC.md`（可攜引擎規格）+ `QUEST_ENGINE_DESIGN.md`（C++/Skyrim 落地）之上。
> 搭配閱讀：`MODDING_COOKBOOK.md`、`COMMONLIBSSE_INDEX.md`、`CLAUDE.md`。
>
> **本次決策（與作者確認）：**
> 1. 先寫設計文件，不動程式碼（引擎尚未實作，mod 程式碼會卡在引擎上）。
> 2. 第一版做**單一宮廷垂直切片**：白漫城（Whiterun / 龍臨堡 Dragonsreach / 領主 Balgruuf）。打通整條循環再複製到其他城。
> 3. 機制分層：**可抽象的部分 → 提案進可攜 SPEC**（§2.2）；**Skyrim 專屬的送達方式 → adapter**（§2.3）。

---

## 0. 概念與核心循環

玩家在各大領主宮廷擔任**大法師**。核心體驗是一條可重複的循環：

```
領主有需求 ──(偶爾)──▶ 寄信召喚你 ──▶ 你前往宮廷 ──▶ 宮廷小劇情/交代任務
      ▲                                                      │
      │                                                      ▼
      └────── 回報、領賞、回到待命 ◀── 完成任務（煉藥／施法解詛咒／滿足要求）
```

任務類型（垂直切片先做前兩種）：
- **施法解詛咒**：對指定目標施指定法術。
- **煉製藥水**：玩家自行煉好指定藥水後交付。
- **滿足領主要求**：取物／護送／談話等雜項（後續擴充）。

**身份分別（横跨多任務的持久狀態，見 §4）：**
- `none` 未受聘 → `visiting` 客座（受聘但不常駐，靠**寄信**召喚）→ `resident` 駐場（常駐宮廷、**有住處**，領主隨時讓**守衛走過來**叫你做事）。

---

## 1. 垂直切片範圍（白漫）

用既有遊戲資產，**不需新 ESP**（與引擎前提一致，見 §5）：

| 元素 | 綁定 |
|------|------|
| 領主 Balgruuf | 既有 NPC（`existing` 綁定，FormID/EditorID）|
| 總管 Proventus | 既有 NPC（召喚劇情的中介）|
| 衛兵 | 既有 NPC（駐場期的「走過來叫你」）|
| 受詛咒的侍從 | **動態生**（`spawn` 綁定，沿用 `NpcGenerator`，`src/NpcGenerator.cpp` 已有）|
| 龍臨堡 | 既有 cell（`location_entered` 偵測）|
| 住處 | 龍臨堡內既有床／marker（駐場後指派）|

> 內容開放問題：白漫原本就有宮廷法師 Farengar。切片先把玩家定位成**客座顧問**（visiting consultant），避開取代 Farengar 的劇情衝突；要不要做「取代/共事」是內容決策（§6）。

---

## 2. 系統 → 引擎詞彙對照

這節是本檔的核心：哪些**現成可用**、哪些要**提案進可攜 spec**、哪些是 **Skyrim adapter 專屬**。

### 2.1 現成可用（核心詞彙 + 既有 Skyrim 擴充，不必新增）

| mod 需求 | 用什麼（見 SPEC §4 / DESIGN §2）|
|----------|------------------------------|
| 宮廷小劇情、台詞、選項 | `dialogues` / `nodes` / `choices`（核心）|
| 任務目標追蹤 | `objectives` + `set_objective_active` / `complete_objective`（核心）|
| 領主給賞（金幣／物品）| `give_gold` / `give_item`（Skyrim 擴充）|
| 煉藥任務：檢查玩家煉好的藥水、交付 | `player_has_item`（條件）+ `remove_item`（交付）|
| 解詛咒任務：要求玩家會某法術 | `player_has_spell`（條件）|
| 導引前往宮廷 | `add_map_marker` + `location_entered`（觸發）|
| 進龍臨堡觸發劇情 | `location_entered`（觸發）|
| 跟領主／總管談話入口 | `activate`（觸發）|
| 生出受詛咒侍從 | `spawn` 綁定 + `spawn_character`（NpcGenerator 模式）|

### 2.2 提案進可攜 SPEC 的新詞彙（generic kernel）

這三項是這個 mod **逼出來的真實引擎缺口**。每項都有與遊戲無關的核心，且能被 SPEC 附錄 A 的 headless CLI 試金石重現 → 屬可攜核心。**（2026-05-23 已 fold 進 `QUEST_ENGINE_SPEC.md`：timers §4.2/§4.3、globals §2.4、deliver_message §4.5；core schema 同步。）**

**(A) 時間排程 / Timers** — 「領主**偶爾**寄信」的節律。引擎目前完全沒有時間驅動。
- 新核心動作 `schedule` `{ after_hours: number, event: string, key?: string }`（或 `at`: 絕對遊戲時間）。
- 新核心觸發 `timer` `{ key: string }`。
- 依賴：把 SPEC §5.7 的**選配** Clock 埠提升為「排程所需」。到期時間（絕對遊戲時間）寫進進度 blob（SPEC §6），重載後續跑。
- 確定性：到期時間存 blob、不靠 `random`。要「隨機間隔」用 `random` + `add_var` 顯式導出（符合 SPEC §8 冪等規則），核心不提供非冪等抖動。
- CLI 試金石：headless adapter 用系統時鐘或手動 `advance-time` 指令模擬 → 可重現，通過可攜檢驗。

**(B) 全域 / 跨任務共享變數** — 大法師在各宮廷的**身份/聲望**横跨多個任務檔，但 SPEC 現有 `vars` 是**任務範圍**。
- 變數參照支援 `global.<name>` 前綴（沿用既有 `set_var` / `add_var` / `var_eq` 等動詞，**不發明新動詞**，最省詞彙）。
- 在**系統層**持久化（不綁單一任務 blob 段）。
- 全域變數須**預先宣告**讓 validator 認得（§4：建議用 `config/quests/_globals.json` manifest）。

**(C) 非同步、需玩家確認的訊息（信件的 generic kernel）** — `show_message` 是**即時、單向**；信件是**延後送達、玩家讀取後才推進**。
- 新增 generic 動作 `deliver_message` `{ to:"player", subject, body, key }` + 事件 `message_ack` `{ key }`。
- 渲染交給 DialoguePresenter / adapter（Skyrim = 信差 + note；CLI = 印出待 ack）。標記 **SHOULD / 選配**（非每個 adapter 都要非同步訊息）。

> **三者串起來 = 完整的「領主偶爾寄信叫你來」**：`schedule`(A) 到期 → `timer`(A) → `deliver_message`(C) → 玩家讀 → `message_ack`(C) → `start_dialogue` / 開目標。

### 2.3 Skyrim adapter 專屬新擴充

遊戲特定的「怎麼送達」，只寫在 adapter（DESIGN §2 的三張表各加一列）。**JSON 端不發明新語法。**

| 種類 | 詞彙 | 參數 | Skyrim 實作 | 風險/保底 |
|------|------|------|------------|-----------|
| 動作 | `deliver_letter` | `{from, subject, body, key}` | 實作 generic `deliver_message` 的 Skyrim 版：信差(courier) 或直接 `AddItem` 一個 `BGSNote` | 信差 hook 有風險 → 保底=直接給 note + 通知 |
| 觸發 | `letter_read` | `{key}` | 玩家讀該 note → 觸發 `message_ack` | menu/讀取事件偵測 |
| 觸發 | `spell_cast_on` | `{character\|form, spell}` | 對指定目標施指定法術。沿用 DESIGN §7 Phase 0「對目標施法 = 開始對話」的偵測 | 施法/命中事件 hook（待研究）|
| 動作 | `force_greet` | `{character}` | 守衛走過來叫你：NPC 尋路到玩家 + 強制 greet/開對話（AI package 或 `move_character`→player + `play_idle` + 開對話）| **高風險** → 保底=直接 MessageBox「衛兵告訴你領主召見」|
| 動作 | `grant_housing` | `{cell\|marker}` | 駐場住處：指派既有床/房間 | 完整「擁有權」較複雜 → 最簡=可睡的 bedroll/marker |

---

## 3. 垂直切片的三支劇情 JSON

| 檔 | id | 作用 |
|----|----|------|
| `cw_whiterun_hire.json` | `cw_whiterun_hire` | **受聘**：首封介紹信或晉見領主 → 成為 `visiting`。設 `global.whiterun_role`。|
| `cw_whiterun_summons.json` | `cw_whiterun_summons` | **召喚 + 一次任務**（可重複循環，完成後 `reset_quest` 復位重排）：寄信 → 前往 → 解詛咒 → 領賞。|
| `cw_whiterun_promote.json` | `cw_whiterun_promote` | **升駐場**：`global.tasks_done` 達標 → 升 `resident` + `grant_housing`，之後改成衛兵 `force_greet` 召喚。|

### 3.1 範例：`cw_whiterun_summons`（同時當新詞彙的可讀性測試）

```json
{
  "id": "cw_whiterun_summons",
  "title": "白漫宮廷的召喚",
  "version": 1,
  "characters": {
    "jarl":   { "bind": "existing", "ref": "JarlBalgruufTheGreater" },
    "victim": { "bind": "spawn", "template": "0x00013BBF", "name": "受詛咒的侍從" }
  },
  "vars": { "delivered": false },
  "objectives": {
    "go_court":   { "text": "前往龍臨堡晉見領主", "state": "inactive" },
    "lift_curse": { "text": "以「驅逐亡靈」對受詛咒的侍從施法", "state": "inactive" }
  },
  "triggers": [
    { "on": "quest_start",
      "do": [ { "schedule": { "after_hours": 48, "key": "wr_summon" } } ] },

    { "on": "timer", "key": "wr_summon",
      "when": { "var_eq": { "var": "delivered", "value": false } },
      "do": [
        { "deliver_letter": { "from": "jarl", "key": "wr_summon",
            "subject": "領主巴爾古夫的來信",
            "body": "大法師，龍臨堡有要事相詢，望速來一晤。" } },
        { "set_var": { "var": "delivered", "value": true } } ] },

    { "on": "letter_read", "key": "wr_summon",
      "do": [ { "set_objective_active": "go_court" },
              { "add_map_marker": { "character": "jarl", "name": "龍臨堡" } } ] },

    { "on": "location_entered", "location": "WhiterunDragonsreach",
      "when": { "objective_state": { "objective": "go_court", "state": "active" } },
      "do": [ { "complete_objective": "go_court" },
              { "start_dialogue": "court_brief" } ] },

    { "on": "spell_cast_on", "character": "victim", "spell": "0x000211EF",
      "when": { "objective_state": { "objective": "lift_curse", "state": "active" } },
      "do": [ { "complete_objective": "lift_curse" },
              { "start_dialogue": "curse_lifted" } ] }
  ],
  "dialogues": {
    "court_brief": {
      "entry": "n0",
      "nodes": {
        "n0": { "speaker": "jarl",
          "lines": [ "大法師，府上一名侍從中了詛咒，群醫束手。",
                     "聽聞你通曉法術，能否解此厄？" ],
          "choices": [
            { "text": "讓我看看那名侍從。",
              "when": { "player_has_spell": { "form": "0x000211EF" } },
              "then": [ { "spawn_character": { "character": "victim" } },
                        { "set_objective_active": "lift_curse" } ],
              "goto": "n1" },
            { "text": "（我還沒學會「驅逐亡靈」）", "end": true } ] },
        "n1": { "speaker": "jarl", "lines": [ "善。她就在偏廳，速去吧。" ], "end": true }
      }
    },
    "curse_lifted": {
      "entry": "m0",
      "nodes": {
        "m0": { "speaker": "jarl",
          "lines": [ "詛咒已解！白漫感念你的相助。" ],
          "choices": [
            { "text": "舉手之勞。",
              "then": [ { "give_gold": { "amount": 500 } },
                        { "add_var": { "var": "global.whiterun_tasks_done", "value": 1 } },
                        { "reset_quest": true } ],
              "end": true } ] }
      }
    }
  }
}
```

> 範例用到的新詞彙：`schedule` / `timer` / `reset_quest`（A + 可重複任務）、`global.*`（B）、`deliver_letter` / `letter_read` / `spell_cast_on`（adapter §2.3）。其餘全是現成核心 + 既有 Skyrim 擴充。
> **循環如何自我延續**：完成時 `reset_quest` 把本任務復位（`delivered` 回 false、目標回 inactive）並重發 `quest_start` → 重新 `schedule` 下一次召喚；跨循環的完成數記在 `global.whiterun_tasks_done`，不受復位影響。
> FormID（`0x000211EF` = Turn Lesser Undead、`0x00013BBF` = 模板 NPC）為**占位**，實作時換真實值並重新挑「解詛咒」用的法術/詛咒設定（§6）。

---

## 4. 身份/聲望狀態模型（global 變數）

横跨多任務、隨存檔持久的「大法師生涯」狀態，用 §2.2(B) 的全域變數承載：

| 全域變數 | 型別/值 | 用途 |
|----------|---------|------|
| `global.whiterun_role` | `"none"\|"visiting"\|"resident"` | 白漫的身份。`visiting`→寄信召喚；`resident`→衛兵召喚 + 有住處 |
| `global.whiterun_tasks_done` | number | 完成任務數，升駐場門檻 |

- 召喚方式分歧由條件 gate：客座走 `deliver_letter`，駐場走 `force_greet`（同一支 summons 任務內用 `var_eq global.whiterun_role` 分支）。
- 升駐場（`cw_whiterun_promote`）在 `global.whiterun_tasks_done` 達標時翻 `role` 並 `grant_housing`。
- **全域變數宣告**：建議新增 `config/quests/_globals.json`（mod 層 manifest，列出全域變數名與初值），讓有效 schema 的 validator 認得 `global.*` 參照——否則依 SPEC §7「未宣告變數＝驗證期報錯」會被擋。每城一組 `global.<hold>_role` / `_tasks_done`，擴充到九大領主時照抄。

---

## 5. 無 CK/ESP 的內容綁定方式

與引擎前提一致（純 JSON + runtime，不開 CK、不出 ESP）：

- **既有 NPC/地點**：領主、總管、衛兵、龍臨堡 cell 全用 `existing` 綁定（`FormUtil::Parse` 的 `"0x..~Skyrim.esm"` 或 EditorID）。
- **動態角色**：受詛咒侍從用 `spawn` 綁定，走 `NpcGenerator`（`src/NpcGenerator.cpp` 已可用）。
- **信件**：adapter 用信差或 `AddItem` 一個 `BGSNote`，內文來自 JSON `body`，不需在 ESP 預建 note。
- **住處**：指派龍臨堡內既有床/marker，不新建 cell。
- 結論：**零新 ESP**，內容全在 `config/quests/*.json` + adapter 邏輯。

---

## 6. 待定 / 開放問題

**引擎決策（已於 2026-05-23 定案並寫進 SPEC）：**
1. ~~可重複/實例化任務~~ → **`reset_quest`**（SPEC §3.1/§4.2）：完成後復位當前任務、不動全域變數，跨循環計數放 `global.*`。捨棄實例 id（`#3`）方案——有了全域變數後 reset 更簡且更可攜。
2. ~~§2.2 三項新詞彙~~ → 已 fold 進 `QUEST_ENGINE_SPEC.md`（timers §4.2/§4.3、globals §2.4、deliver_message §4.5）+ 更新 `config/schema/quest.core.schema.json`。

**Skyrim 可行性 spike（風險由高到低）：**
3. `force_greet`（衛兵走過來叫你）：AI package/尋路到玩家 + 強制 greet 是 R&D，**保底直接 MessageBox**（與原生對話 spike 同哲學，見 DESIGN §5）。
4. `spell_cast_on`：偵測「對某目標施某法」要哪個 hook/事件，待研究（沿用 DESIGN §7 Phase 0 的「對目標施法」偵測）。
5. `deliver_letter` 的信差送達 vs 直接給 note：信差 hook 風險高 → 保底直接 `AddItem` + 通知。

**內容/lore 決策：**
6. Farengar 處理：客座顧問（避衝突，切片採此）／取代／共事？
7. 「解詛咒」的法術與詛咒設定：占位用 Turn Undead，要挑合理的法術↔詛咒對應（驅散、淨化、解咒卷軸…）。
8. 住處「擁有權」深度：可睡 bedroll／指派房間／完整 owned cell。

---

## 7. 下一步（待你拍板）

1. ~~同意 §2.2 → fold 進 SPEC + 更新 core schema~~ ✅ **已完成（2026-05-23）**：timers / globals / deliver_message / `reset_quest` 已寫進 `QUEST_ENGINE_SPEC.md` + `config/schema/quest.core.schema.json`。
2. 引擎 **Phase 0 骨架**（DESIGN §7：最小狀態機 + 能力埠 + `MessageBoxPresenter`）是跑任何切片內容的**前置**。
3. 撰寫三支切片任務 `config/quests/cw_whiterun_*.json` + `_globals.json`。
4. **先用 CLI harness 驗證可攜部分**（schedule/timer/globals/對話流程，不需 Skyrim）；Skyrim 專屬詞彙（letter/spell_cast_on/force_greet）再各自 spike。
