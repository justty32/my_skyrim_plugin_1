# INDEX — my_skyrim_plugin_1（DaylightDungeon）`wf/` 地圖

my_skyrim_plugin_1 = **DaylightDungeon：Skyrim SE/AE/GOG/VR 的 SKSE 插件（CommonLibSSE-NG）＋其建置基礎設施**（靜態 CRT 的 CMake/vcpkg preset、Linux clang-cl 交叉編譯、MO2 打包、GitHub Actions CI）。**repo 本體的導航是根 [AGENTS.md](../AGENTS.md) 的「Where to look」表**；本檔只描述 `wf/`（2026-10-26 非侵入式導入的 wf kernel），每列一句話＋連結。

## `wf/` 佈局

| 路徑 | 內容 |
|------|------|
| `workflows/` | 工作流（派發見 [WORKFLOWS.md](WORKFLOWS.md)；共享區 [workflows/common/](workflows/common/README.md)）。dev-env／testing／conventions／code-map 都是索引，細節導回根 `BUILD.md`、`FOR_AGENT.md`、`AGENTS.md`、`PITFALLS.md` |
| `tools/` | kernel 工具：`wf-lint.sh`（檢查）、`tabledb.py`（資料檔 CRUD／連結）、`find_big_lists.py`、`fix_moved_links.py`；資料檔契約見 [workflows/common/data-files.md](workflows/common/data-files.md) |
| 根 `.claude/commands/` | slash 指令適配層（`/wf-lint`）；Claude Code 只讀專案根的這層，所以留在根 |
<!-- wf-insert:INDEX -->

## 頂層文件

| 檔案 | 角色 |
|------|------|
| [WORKFLOWS.md](WORKFLOWS.md) | 派發器：意圖 → 工作流入口 |
| [STRUCTURE.md](STRUCTURE.md) | 結構整理參考（被動）：分層、膨脹即拆、四級成長、archive、工作流形式 |
| [SESSION-LOG.md](SESSION-LOG.md) | 本 repo 的 open 進度 |
| [母 repo WAIT_USER.md](../../../WAIT_USER.md) | 等使用者親自做 / 驗證的事，以母 repo 那份為準，本 repo 不另開 |
