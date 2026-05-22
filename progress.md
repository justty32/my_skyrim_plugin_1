# 進度 / 接續筆記（宮廷大法師 mod + Quest Engine）

> 交接用 scratch 檔。冷啟動看這份就能接續。
> 分支：`feature/court-wizard`　最後更新：2026-05-23

## 一句話現況

可攜 quest engine 的 **Phase 0 骨架已完成並驗證**（無遊戲、原生 clang 跑通整條召喚循環）；下一步是 **Phase 1 — Skyrim adapter**。宮廷大法師 mod 是這個引擎的第一個內容。

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
- Phase 0 先用 **CLI harness** 無遊戲驗證，**不先**做 `MessageBoxPresenter`（換 presenter 不動核心）。
- `random` 在 PRF 演算法定案前**不實作**（SPEC §9 #1）。

## 下一步 — Phase 1（Skyrim adapter）

1. 把 core 登錄進 `cmake/sourcelist.cmake` / `headerlist.cmake`，並確保 **PCH（`RE::Skyrim.h`）不汙染 core TU**（core 要保持零遊戲依賴）。
2. 實作六個能力埠到 `src/skyrim/`（見 `QUEST_ENGINE_DESIGN.md` §6 預定分層）：
   - `MessageBoxPresenter`（保底 DialoguePresenter）
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
