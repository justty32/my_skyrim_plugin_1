# user — 使用者偏好與確認邊界

[common/README](README.md)

agent 不用重猜的事。always-on 鐵律在 AGENTS.md，這裡是**這位使用者**的偏好——改了改這裡，不改鐵律。

**何時用**：使用者說了偏好、講定了確認邊界（哪些事可以直接做、哪些一定先問）、或講定分支慣例／時區／語言時，記到這裡。
**何時不用**：一次性的小事不必記（例如「這次先不要跑測試」）——那是當下的指示，不是長期偏好；長期知識歸屬層的原則見 [STRUCTURE](../../STRUCTURE.md)。

## Done when

對應的那一列已更新（或新開一列），沒有留下與新偏好矛盾的舊值。

| 項目 | 設定 |
|------|------|
| 語言 | 回覆與文件一律**繁體中文**；技術專名（mod 名、API、指令、欄位）留原文；`src/` 程式碼註解英文（照現況）|
| 分支慣例 | 直接 commit `main`；建／刪分支前先看根 [BRANCHES.md](../../../BRANCHES.md)；**push 要使用者當場確認**（依鐵律 2）|
| 直接做、不用問 | 改文件、跑唯讀指令（`git status`／`grep`／`find`／查 Nexus）、跑 [testing](../testing.md) 的 agent 列 |
| 一定先問 | 刪檔、動工作區 `instance/`（MO2 profile／load order／已裝 mod）、安裝依賴、開新的大型工作（push 與對外動作依鐵律 2）|
| 回覆風格 | 通用風格見 [reply-style.md](reply-style.md)；短、先結論。使用者問「**要不要**」時不要只回「不必」，附上可執行的判準（門檻數字）與後果，讓他能改數字 |
| 時區 | Asia/Taipei |

2026-10-21 使用者裁示：所有負責實質工作的 agent 必須先親自讀過適用的 `wf/` 規矩，不能只靠領隊摘要；簡單工作或雜活例外。

領域詞彙常猜錯 → 開 `glossary.md`（見 [common/README](README.md)）。
