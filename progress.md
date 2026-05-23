# 進度 / 接續筆記（宮廷大法師 mod + Quest Engine）

> 交接用 scratch 檔。冷啟動看這份就能接續。
> 分支：`feature/court-wizard`　最後更新：2026-05-23

## 一句話現況

**完整功能集已整合 + 過一輪 QA 修復**（branch `feature/court-wizard` @ `7488252`）：可攜 core（audit 強化 + validate + 防 crash + SPEC §6 進度持久化）+ Skyrim adapter 六埠 + 可恢復式 MessageBox 對話 + 煉藥 spike + **四種 runtime 程序生成（NPC / 物品 / 室內房間 / 室外城堡）** + 中央 `'TPL1'` co-save 持久化，全部編進**一個 cross-compile DLL**（1.72MB PE32+）。**整包尚未 in-game 測試**（Linux 交叉編譯）。下一步＝**人類照 `TESTING_GUIDE.md` 進遊戲實測**。成果包：`dist/CourtWizardSuite-0.0.1.zip`。

## 本次自主 session 成果（2026-05-23，全部 commit + 整合，HEAD `7488252`）

提交鏈（`8f0318a`→`7488252`）一線，每步都 ff/merge + 重建 DLL + 刷新成果包：
- **Phase 1**：cmake 登錄 core + PCH 隔離、可恢復式對話、Skyrim adapter 六埠、quest engine audit 強化。
- **程序生成**：①`generate_interior`（房間）②`generate_structure`（城堡）③`generate_npc`（`TESNPC_` mint+Copy）④`generate_item`（WEAP/ARMO/MISC；WEAP/MISC 因 `TESForm::Copy` 是 no-op 改用 component 深拷貝）。各有示範法術（`C++: Generate Room`/`Conjure Keep`/`Rearrange Furnishings`/`Conjure NPC`/`Conjure Item`）+ JSON adapter 動作。
- **持久化**：中央 co-save 派發器 `src/skyrim/CoSave.{h,cpp}`（單一 `SetUniqueID('TPL1')` + AddHandler API），模組各用 record type：`'QEST'`（劇情進度/全域/計時器，SPEC §6 blob）、`'GNPC'`、`'PRGN'`、`'GITM'`。全為**邏輯層級**持久化（存 recipe + plugin-FormID + 原點/seed，重載重建；**絕不存 0xFF 動態 FormID**）。煉藥藥水**尚未**接 co-save。
- **真實 FormID**：移除捏造的 `WRC*`，NPC 模板換成已驗證土匪 `0x0001BCD8`，家具/物品用已驗證 FormID（部分仍 MEDIUM 信心，見 `REVIEW_FINDINGS.md`）。
- **QA**：獨立審查 agent 出 `research/REVIEW_FINDINGS.md`（1 Critical + 2 High + 3 Medium 真 bug）→ **全部修復**（C1 OnRevert 脫離主執行緒、H1 物品讀檔累加、H2 RangedData aliasing、M1 reset-in-dialogue、M2 start/import 順序、M3 clutter scatter）→ CLI 測試 63→**67 passed**。
- **文件**：`research/` 9 份（含 3 procgen 可行性 + 2 spike 發現 + audit + review）；`TESTING_GUIDE.md`（逐步 in-game 測試手冊）；`使用說明書.md`（中文成品說明）。
- **成果包** `dist/CourtWizardSuite-0.0.1.zip`：DLL + config 樹 + 9 research + 3 design-docs + 使用說明書 + 測試手冊。

## 下一步（依 in-game 測試結果決定）

1. **先做**：照 `TESTING_GUIDE.md` 進遊戲測（最小集：①冒煙 ②生成物品+多次存讀不累加 ③劇情對話含按鈕對應）。把「過/不過 + log 尾段 + 截圖」回報。
2. **in-game 才現形的待修**（可能項）：MessageBox 按鈕索引對應、procgen snap 幾何/浮空、佔位 FormID resolve 不到（用 console `help` 換真 ID 進 recipe）、co-save 重建路徑是否真重現、煉藥數值是否等於 vanilla 選單。
3. **未做的功能**：煉藥藥水的 co-save；`spell_cast_on` 真實 hook（現用 F10 debug 鍵替代，候選方案在 `SkyrimEvents.cpp` 註解）；adapter 高風險動詞仍 stub（start_combat/add_shout/teleport_player…）；原生對話選單 spike；`random`/PRF。
4. **debug 熱鍵**：F9 輪詢計時器、F10 觸發 spell_cast_on、F11 煉藥（正式化時移除）。

## 多 agent 編排教訓（重要，見 memory `feedback_worktree_agents`）

- worktree 隔離下 **agent cwd = 主樹**，會誤寫主樹再自還原 + 把成果提交到 `worktree-agent-<id>` 分支 → **整合一律從分支來**（ff / merge / 取檔 + 套 fence 增修）。
- **code agent 序列化**（碰共用 `cmake`/`plugin.cpp`/`SkyrimActions` 者不可並行）；檔案範圍互斥的可並行（如「core/adapter」vs「config/procgen」）；純研究 agent（只寫新 `research/*.md`）可全並行。
- 共用檔編輯用 `// >>> <tag>` / `// <<< <tag>` fence 標記，agent 回報確切增修，合併容易。
- agent 可能被 **session limit 中斷**（結果是 limit 訊息、0 commit、無成果）→ 額度回來後重跑。
- 舊 worktree（`.claude/worktrees/`）harness 鎖定管理，**勿強制移除**，session 結束自動清；分支留著當備份。

## （歷史）Phase 1 起步過程

- **core 登錄 cmake + PCH 隔離**（DESIGN §6 step 1）：`cmake/sourcelist.cmake` 新增 `core_sources`（`src/core/QuestEngine.cpp`）、`cmake/headerlist.cmake` 加 core headers；`CMakeLists.txt` 在 PCH 行後 `set_source_files_properties(${core_sources} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)`，確保 `RE/Skyrim.h` 不被強制塞進 core TU。**已驗證**：`compile_commands.json` 中 `QuestEngine.cpp` 無任何 PCH flag，`plugin.cpp` 有；cross-compile DLL 正常編譯 + link。
- **對話改可恢復式**（決策見下）：`Ports.h` 的 `IDialoguePresenter::presentNode` 改成 **display-only（回傳 void，不阻塞）**；`QuestEngine` 新增 `submitChoice(int)` + `awaitingChoice()`，內部把同步 while 迴圈拆成 `startDialogue → presentCurrentNode → (await) → submitChoice → presentCurrentNode...`，新增 `endDialogue()`。CLI harness 改用 `drainChoices()` 從 stdin 讀選擇餵回。**已驗證**：原驗證情境輸出**逐字節相同**，且「對話中 `reset_quest`」、選 `end:true`、取消（無效輸入）三個 edge case 皆正確。
- 兩條 build 都綠：`./scripts/build_cli.sh`（原生 clang）+ `cmake --build build/release-clang-cl-linux`（cross-compile DLL）。

## 已完成（本分支兩個 commit）

1. `fe05397 docs:` — 設計定案
   - `COURT_WIZARD_DESIGN.md`（新）：宮廷大法師 mod，**白漫單一宮廷垂直切片**，對應到引擎；通用機制 → SPEC，Skyrim 送達方式 → adapter。
   - `QUEST_ENGINE_SPEC.md`：加入 `schedule`/`timer`（§4.2/4.3）、`global.*`（§2.4）、`deliver_message`/`message_ack`（§4.5 標準擴充）、`reset_quest`（§3.1/4.2）；持久化/驗證/確定性/CLI 附錄同步。
   - `config/schema/quest.core.schema.json`：加 `schedule`/`reset_quest`/trigger `key`。
   - `QUEST_ENGINE_DESIGN.md`：新詞彙的 Skyrim 對應。

2. `81717a4 feat(core):` — Phase 0 引擎骨架
   - `src/core/`（**零 RE::/SKSE::**，只 std + nlohmann-json）：`QuestState.h`、`Ports.h`、`QuestEngine.{h,cpp}`。核心狀態機 + 條件/動作/觸發 + 同步對話流程 + `schedule`/`timer` + `global.*` + `reset_quest`；未知動詞轉交能力埠。
   - `tools/cli_harness/main.cpp`：headless CLI adapter（SPEC 附錄 A）。
   - `config/quests/demo_court_wizard.json`：demo 劇情。
   - `scripts/build_cli.sh`：原生建置。

## 怎麼接著跑（驗證 Phase 0）

```bash
./scripts/build_cli.sh                       # → build/cli/qe_cli（clang，免 Skyrim）
printf 'time 48\n1\ncast\n1\nstate\nquit\n' | ./build/cli/qe_cli
```
預期：等 48h→收信開對話→選 1 接任務→`cast` 施法解咒→領主給賞→`reset_quest` 重排，且 `global.whiterun_tasks_done` 跨重置保留。
REPL 指令：`time N` / `cast` / `fire <on> [k v]` / `state` / `quit`。

## 已定案的關鍵決策（別再翻案）

- 範圍：先做**白漫一個宮廷**垂直切片。
- 分層：通用機制進**可攜 SPEC**，Skyrim 專屬（信差送信、找守衛、施法偵測）留 **adapter**。
- 可重複任務用 **`reset_quest`**（復位當前任務、不動全域變數，跨循環計數放 `global.*`）；**捨棄**實例 id 方案。
- Phase 0 先用 **CLI harness** 無遊戲驗證，**不先**做 `MessageBoxPresenter`。
- **對話採可恢復式（2026-05-23 定案）**：`presentNode` display-only、不阻塞；引擎用 `submitChoice()` 推進。原因：Skyrim MessageBox 是非同步 callback，主執行緒同步阻塞會凍結遊戲，且 worker 執行緒阻塞會讓 ActionRunner 的 `RE::` 呼叫脫離主執行緒。presenter 仍是純顯示 sink，**核心對話狀態機這次有動**（從 while 迴圈改成 yield/resume），這是正確做法、非翻案。
- `random` 在 PRF 演算法定案前**不實作**（SPEC §9 #1）。

## 下一步 — Phase 1（Skyrim adapter）

1. ~~把 core 登錄進 cmake + PCH 隔離~~ ✅ 完成（見上「Phase 1 進行中」）。
2. 實作六個能力埠到 `src/skyrim/`（見 `QUEST_ENGINE_DESIGN.md` §6 預定分層）：
   - `MessageBoxPresenter`（保底 DialoguePresenter）— 因 core 已可恢復式，做法明確：`presentNode` 顯示 MessageBox（`RE::MessageBoxData`，choices 當按鈕），callback 觸發時呼叫 `engine.submitChoice(idx)`；終端節點（無 choices）用通知/字幕顯示。
   - Clock = `RE::Calendar`
   - EntityResolver（existing = FormUtil::Parse / EditorID；spawn = NpcGenerator）
   - EventSource（`spell_cast_on` 等，hook 待研究）
   - ActionRunner / ConditionEvaluator（DESIGN §2 對照表）
   - `deliver_letter` / `letter_read`（信件，保底直接給 BGSNote）
3. co-save 持久化（SPEC §6：全域變數 + 待發計時器；DESIGN §4）。
4. 把切片三支劇情 `config/quests/cw_whiterun_*.json` + `_globals.json` 寫出來（COURT_WIZARD_DESIGN.md §3/§4）。

## 開放問題

- `random` PRF 演算法未定（SPEC §9 #1，阻擋 `random`）。
- `spell_cast_on` 用哪個 hook/事件偵測「對目標施法」，待研究（DESIGN §6 待定）。
- `force_greet`（守衛走過來叫你）高風險 → 保底 MessageBox（COURT_WIZARD §6）。
- 內容：白漫原宮廷法師 Farengar 怎麼處理（切片暫採「客座顧問」避衝突）。

## 檔案地圖

| 檔 | 角色 |
|----|------|
| `QUEST_ENGINE_SPEC.md` | 可攜規格（真正可攜的產物，MUST/SHOULD）|
| `QUEST_ENGINE_DESIGN.md` | C++/Skyrim 落地 + 分期路線（§7）|
| `COURT_WIZARD_DESIGN.md` | 宮廷大法師 mod 內容/系統設計 |
| `config/schema/quest.core.schema.json` | 核心詞彙 JSON Schema |
| `src/core/` | 可攜引擎（Phase 0 完成）|
| `tools/cli_harness/` | headless 驗證 adapter |
| `config/quests/demo_court_wizard.json` | demo 劇情 |
| `research/` | 與 quest engine 無關的**未來功能可行性分析**（NPC 行為深改 / 3D 物理煉藥 / 無 navmesh 尋路），header 已查證，僅分析未實作 |
