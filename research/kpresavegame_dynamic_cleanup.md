# 存檔前移除動態 (0xFF) actor/ref 的安全設計

> 目標：在遊戲**寫入 `.ess` 之前**，把我們在 session 內鑄造（mint）出來的動態（`0xFF......`）
> actor / ref 全部移除，讓 `.ess` 不再持久化「懸空的動態 base」，從而根治
> **跨 session 冷重啟讀檔 crash**。本文只處理存檔側清理設計；within-session 讀檔的 re-adopt 路徑不變。

本設計已按主題拆分：

- [時序與方案比較](kpresavegame_timing_and_options.md)：SKSE 存檔時序、round-trip 與三種掛點方案。
- [清理範圍與實作計畫](kpresavegame_cleanup_api_plan.md)：registry 安全護欄、對外 API 與逐檔落地步驟。
- [風險與查證索引](kpresavegame_risks_and_references.md)：開放風險、TODO 與已核對的標頭／程式碼位置。

所有簽章皆對照本 repo 內 vendored 的真實 SKSE / CommonLibSSE-NG 標頭驗證，不憑記憶。
