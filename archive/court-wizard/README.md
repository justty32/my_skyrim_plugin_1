# archive/court-wizard — 宮廷大法師 mod 的設計與交接文件（快照）

> ⚠️ **這是 2026-05-26 的快照，不是現況。** 檔案原樣搬自 `feature/court-wizard` 分支，未改內容。

## 這批是什麼

`feature/court-wizard` 是一條**還編得過、且實機驗證過**的大分支（34 commits）：可攜 quest engine（`src/core/`）+ Skyrim adapter（`src/skyrim/`）+ 程序化生成（NPC／物品／室內房／城堡）+ headless CLI harness。**分支本身沒有廢掉**，程式碼仍在 `origin/feature/court-wizard`（`992d7ab`）。

這裡只搬**設計與交接文件**上 `main`，理由是：那些知識不該被鎖在一條沒人 checkout 的分支上。程式碼留在分支。

| 檔 | 是什麼 |
|---|---|
| `COURT_WIZARD_DESIGN.md` | mod 設計稿（劇情迴圈、任務結構）|
| `TESTING_GUIDE.md` | 逐步實機測試流程（熱鍵、安裝管道）|
| `progress.md` | 交接筆記 ⚠️ **內容大半已過時**，見下 |
| `project_status.html` / `court_wizard_deep_dive.html` | 當時的儀表板快照 |

## ⚠️ `progress.md` 的過時警告

那份 progress 檔混了兩件事，其中一件**嚴重過時**：

- **court-wizard／quest-engine 本身的進度** — 大致仍準（2026-05-24 實機驗證 `cw_whiterun_summons` 全流程通過；末尾記的「改動未 commit」後來已 commit 成 `5d94be9`／`992d7ab`）。
- **ModForge 的進度（It.7–It.10）** — ⚠️ **完全過時，別信**。那是 ModForge 剛開始的狀態；ModForge 現在是獨立成熟專案（`../ModForge`，1013 測綠、worldspace／NPC／quest／語音全線、還長出 Godot 編輯器與遊戲內採集橋兩個外掛 repo）。要看 ModForge 現況去它自己的 `SESSION-LOG.md`。

裡面提到的 `/home/lorkhan/repo/ModForge` 路徑也早就搬到 `~/repo/moddings/skyrim/projects/ModForge`。

## 已併回 main 的部分

搬這批的同時，`feature/court-wizard` 上**嚴格較新**的三份檔案已取代 `main` 的舊版（同日但晚 30 分鐘～14 小時，且內容是超集）：

- `QUEST_ENGINE_SPEC.md` — 多了 globals（§2.4）、`schedule`/`timer`（§4.2/4.3）、`reset_quest`、非同步訊息標準擴充（§4.5）、Clock 埠升級為條件必需（§5.7）
- `QUEST_ENGINE_DESIGN.md`
- `config/schema/quest.core.schema.json` — 隨 `ab8c84a` 的 validate 強化更新

該分支的 12 份 `research/*.md` 也全部搬進 `main` 的 `research/`。
