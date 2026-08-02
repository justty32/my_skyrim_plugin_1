# 劇情/對話引擎 — C++ 參考實作 + Skyrim Adapter

> 狀態：**設計階段，尚未實作**。
> **可攜契約（格式、狀態機語意、詞彙、能力埠、持久化、一致性）在 `QUEST_ENGINE_SPEC.md`。本檔只談這個規格在 C++/SKSE/Skyrim 上的落地。**
> 搭配閱讀：`MODDING_COOKBOOK.md`（本專案核心模式）、`COMMONLIBSSE_INDEX.md`（有哪些 class）、`PITFALLS.md`（編譯雷區）、`CLAUDE.md`（專案規則）。

C++/SKSE 是 spec 的**參考實作之一**；Skyrim 是它的第一個 adapter。

---

## 1. 兩個 target：可攜核心 + Skyrim adapter

```
src/core/   ── 可攜核心 Runtime：實作 SPEC 的狀態機/條件/動作/對話流程/持久化格式
            ── 鐵則：一行 RE:: / SKSE:: 都不能有，只依賴 std + nlohmann-json
src/skyrim/ ── Skyrim adapter：唯一 include RE::Skyrim.h / SKSE，實作所有能力埠（§5 of SPEC）
```

`src/core/` 理想上是可獨立抽出的 library，換遊戲時整包搬走、只重寫 adapter。

能力埠 → Skyrim 實作對照：

| SPEC 能力埠 | Skyrim adapter 實作方式 |
|-------------|------------------------|
| EntityResolver | `LookupByEditorID` / `FormUtil::Parse`（FormID~mod）；spawn 走 `NpcGenerator::SpawnNpc` 模式（`PlaceObjectAtMe`）|
| ActionRunner | 見 §2 動詞對照表 |
| ConditionEvaluator | 見 §2 條件對照表 |
| EventSource | `ScriptEventSourceHolder` + `BSTEventSink<T>`（`TESActivateEvent` / `TESDeathEvent` / `TESContainerChangedEvent` / `TESCellAttachDetachEvent`）|
| DialoguePresenter | §3：MessageBox（保底）/ 原生 DialogueMenu（spike）/ Scaleform（選配）|
| PersistenceBackend | `SKSE::SerializationInterface`（co-save）—— 見 §4 |
| Clock / Logger / RNG | `RE::Calendar` / `SKSE::log` / std RNG |

---

## 2. Skyrim adapter 宣告的擴充詞彙

核心詞彙由 `src/core/` 直接實作（見 SPEC §4.1–4.3）。下列是 **Skyrim adapter 對核心宣告**的擴充詞彙（SPEC §4.4），及其對應 API。

> （2026-05-23 SPEC 更新）新增的核心詞彙 `schedule`/`timer`（計時器）、`global.*`（全域變數）、`reset_quest`（可重複任務）一樣由 `src/core/` 實作，**不是** adapter 擴充。Skyrim 端只需：Clock 埠用 `RE::Calendar`（§5.7 由選配升必需），全域變數與待發計時器隨 co-save 持久化（§4）。SPEC §4.5 的**標準擴充** `deliver_message`/`message_ack`，Skyrim 實作為信件——見 §2.2 `deliver_letter` / §2.4 `letter_read`。

### 2.1 實體綁定描述（characters 區塊由 adapter 解讀）

```json
"characters": {
  "merchant": { "bind": "existing", "ref": "BelethorServices" },
  "thug":     { "bind": "spawn", "template": "0x00000007", "name": "可疑的打手" }
}
```
- `existing`：`ref` 走 `FormUtil::Parse`（`"0x..~Skyrim.esm"`）或 EditorID。身分穩定。
- `spawn`：`template` 模板 NPC 動態生（`NpcGenerator` 模式），`name` 覆寫顯示名。
- 解析結果（FormID / spawned handle）序列化進 co-save；讀檔後重解析，spawned 失效依政策重生或標記中斷。

### 2.2 擴充動作（ActionRunner）

| 動詞 | 參數 | Skyrim API |
|------|------|-----------|
| `give_item` / `remove_item` | `{form, count}` | `Actor::AddObjectToContainer` / `RemoveItem` |
| `give_gold` / `remove_gold` | `{amount}` | 同上（Gold001）|
| `add_spell` / `remove_spell` | `{form}` | `Actor::AddSpell`（見 `NpcGenerator`）|
| `add_shout` | `{form}` | `ActorEquipManager::EquipShout` |
| `spawn_character` | `{character}` | EntityResolver spawn → `PlaceObjectAtMe` |
| `move_character` | `{character, to}` | `SetPosition`（to: `player`/marker/另一 character）|
| `teleport_player` | `{to}` | `MoveTo` / `ObjectUtil::Transform::TranslateTo` |
| `start_combat` | `{character, against}` | `Actor::StartCombat` |
| `set_relationship` | `{character, level}` | relationship rank |
| `play_idle` | `{character, idle}` | `AnimUtil::Idle::Play`（util.h 已有）|
| `add_map_marker` | `{pos, name}` | 動態 map marker |
| `play_sound` | `{form}` | `BSAudioManager` |
| `deliver_letter`（實作 SPEC §4.5 `deliver_message`）| `{from, subject, body, key}` | 信差(courier) 或直接 `AddItem` 一個 `BGSNote`；信差 hook 風險高 → 保底直接給 note + 通知 |

### 2.3 擴充條件（ConditionEvaluator）

| key | 參數 | Skyrim API |
|-----|------|-----------|
| `player_level_gte` / `_lte` | number | `PlayerCharacter::GetLevel` |
| `player_gold_gte` | number | 玩家金幣計數 |
| `player_has_item` | `{form, count}` | inventory 查詢 |
| `player_has_spell` | `{form}` | `Actor::HasSpell` |
| `character_alive` / `character_dead` | `{character}` | `Actor::IsDead` |
| `in_location` | `{location}` | `PlayerCharacter::GetCurrentLocation` |
| `time_of_day` | `{from, to}` | `RE::Calendar` |

### 2.4 擴充觸發（EventSource）

| `on` | 過濾 | 事件 |
|------|------|------|
| `activate` | `{character}` | `TESActivateEvent`（談話入口）|
| `actor_death` | `{character}` | `TESDeathEvent` |
| `item_acquired` | `{form}` | `TESContainerChangedEvent` |
| `location_entered` | `{location}` | `TESCellAttachDetachEvent` + 位置檢查 |
| `letter_read`（觸發 SPEC §4.5 `message_ack`）| `{key}` | 玩家讀該 `BGSNote` → 發 `message_ack` |

> 新增擴充詞彙 = 改這三張表 + adapter 對核心多宣告一項 + 實作對應埠。**JSON 端不發明新語法。**

---

## 3. DialoguePresenter 三個 backend

SPEC §5.5 的對話呈現埠，Skyrim 有三個可換實作：

| backend | 體驗 | 工程量 | 風險 | 角色 |
|---------|------|--------|------|------|
| **MessageBoxPresenter** | 彈窗選項 + 字幕/通知顯示台詞 | 低 | 幾乎無 | **保底，先用它把整條流程打通** |
| **NativePresenter** | 原生對話選單 | 高 | 高（見 §5）| spike，成功才接 |
| **ScaleformPresenter** | 自訂沉浸 UI | 很高 | 中 | phase 4 選配 |

換 backend MUST NOT 影響核心或劇情 JSON。

---

## 4. 持久化：co-save 實作 PersistenceBackend

- 用 `SKSE::SerializationInterface`（見 `COMMONLIBSSE_INDEX.md` §SKSE）。
- 核心給一個版本化進度 blob（SPEC §6），adapter 把它寫進 .skse co-save、讀回。blob 內含 `master_seed`（供 SPEC §8 的 `random` 確定性導出）；adapter 不需理解內容，原樣存取即可。
- **鍵一律穩定字串 ID，絕不存動態 FormID 當主鍵**（讀檔後會變）。
- 全域變數（SPEC §2.4）與待發計時器（SPEC §4.2）一併入 co-save：全域變數鍵用變數名於系統層；計時器存（任務 id、key、絕對到期遊戲時間，由 `RE::Calendar` 換算）；未確認的信件（SPEC §4.5）亦持久化待 `letter_read`。
- 讀檔流程：先重載 JSON 定義，再套用 blob 進度。改 .json 文字內容不弄壞舊存檔。

---

## 5. 原生 DialogueMenu — spike 研究筆記（Skyrim 專屬）

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
src/core/QuestEngine.{h,cpp}       載入 JSON、持有狀態機、總分派（無 RE::）
src/core/QuestState.h              執行期狀態結構（vars/objectives/節點）
src/core/Conditions.{h,cpp}        SPEC §4.1 核心條件求值
src/core/Actions.{h,cpp}           SPEC §4.2 核心動作執行
src/core/Triggers.{h,cpp}          SPEC §3.3 觸發分派
src/core/Ports.h                   SPEC §5 能力埠的 C++ 抽象介面
src/core/Persistence.{h,cpp}       SPEC §6 進度 blob 序列化（格式）

src/skyrim/SkyrimAdapter.{h,cpp}   綁定核心 ↔ 遊戲，向核心宣告擴充詞彙
src/skyrim/SkyrimEntities.{h,cpp}  EntityResolver（§2.1）
src/skyrim/SkyrimActions.{h,cpp}   ActionRunner（§2.2）
src/skyrim/SkyrimConditions.{h,cpp} ConditionEvaluator（§2.3）
src/skyrim/SkyrimEvents.{h,cpp}    EventSource（§2.4）
src/skyrim/CoSavePersistence.{h,cpp} PersistenceBackend（§4）
src/skyrim/dialogue/MessageBoxPresenter.{h,cpp}  保底
src/skyrim/dialogue/NativePresenter.{h,cpp}      §5 spike

config/quests/*.json               劇情定義
config/schema/quest.core.schema.json  核心詞彙 JSON Schema（adapter 擴充與之合併成有效 schema）
```

---

## 7. 分期路線

- **Phase 0 — 骨架** ✅（2026-05-23，`feature/court-wizard`）：`src/core/`（`QuestState.h` / `Ports.h` / `QuestEngine.{h,cpp}`，零 RE::/SKSE::）實作核心狀態機 + 條件/動作/觸發 + 同步對話流程 + `schedule`/`timer` + 全域變數 + `reset_quest`。能力埠先由 **headless CLI harness**（`tools/cli_harness/`，SPEC 附錄 A）實作以無遊戲驗證；`MessageBoxPresenter` 待接 Skyrim adapter 再補。Demo `config/quests/demo_court_wizard.json` 跑通整條召喚循環（信→對話→「對 victim 施法」事件解咒→領賞→`reset_quest` 重排，`global.whiterun_tasks_done` 跨循環保留）。建置：`scripts/build_cli.sh` → `build/cli/qe_cli`。
  - **注意**：core 目前只由該腳本原生（clang，Manjaro）編譯，**尚未**登錄 `cmake/sourcelist.cmake`/`headerlist.cmake`——接 Skyrim adapter、並確保 PCH（`RE::Skyrim.h`）不汙染 core TU 時，於 Phase 1 登錄。
  - Conditions/Actions/Triggers 暫合在 `QuestEngine.cpp`，成長後再依本檔 §6 拆檔。`random`（待 PRF）、持久化（§4）未實作。
- **Phase 1 — 核心**：完整核心 + Skyrim 擴充詞彙 + co-save + EntityResolver 兩種綁定 + 目標狀態。
- **Phase 2 — 任務化**：世界事件觸發（殺/到/拿）+ 自製日誌追蹤 + 多任務並行 + 有效 schema validator（嚴格錯誤訊息）。
- **Phase 3 — 原生對話 spike**：依 §5 go/no-go；成功才接 `NativePresenter`。
- **Phase 4（選配）**：Scaleform 對話 UI；把 `src/core/` 抽成獨立 library 驗證可攜性。

---

## 8. 待定問題（之後決策）

- ~~多任務優先序~~ → 已暫定（SPEC §8）：3 級 `priority` + 先搶先贏；強制執行細節待實作期再定。
- ~~第二 adapter 試金石~~ → 已定：headless CLI harness（SPEC 附錄 A），同時當離線 validator；實作語言待定。
- **`PRF` 演算法未定案（高優先，見 SPEC「未定事項」）**：`random` 確定性導出用的雜湊尚未釘死，未定前 `random` 不可實作。
- LLM 生成的驗證：核心 schema 已生（`config/schema/quest.core.schema.json`，**暫定可能改**）；離線 validator vs 載入時驗證的時機待定。
- 對話進行中存檔的行為（鎖存檔？還是允許並在讀檔後重開對話？）。
- spawned 角色讀檔失效的預設政策（重生 / 中斷 / 視任務標記）。
- 是否要 MCM 風格的任務日誌 UI（vs 純通知）。
