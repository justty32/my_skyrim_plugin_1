# BRANCHES — 現行分支

本 repo 現行產品線只保留以下兩條；建置與功能現況以各分支 HEAD 為準。

| 分支 | 現況 | 處置 |
|---|---|---|
| `main` | DaylightDungeon 主線（FollowLight、AmbientBoost、NpcGenerator） | 日常開發與文件正典 |
| `origin/feature/court-wizard` | Quest engine、Skyrim adapter 與程序化生成的平行產品線 | 程式碼只在該分支，保留且不要刪除 |

舊 spike 的可用原始碼已收在 [`vendor/`](vendor/README.md)，不參與建置；停止維護的 court-wizard 設計與交接快照在 [`archive/court-wizard/`](archive/court-wizard/README.md)，不代表現況。
