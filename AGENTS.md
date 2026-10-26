# AGENTS.md

Guidance for any coding agent (Claude Code, Gemini, Codex, …) working in this repository. [CLAUDE.md](CLAUDE.md) and [GEMINI.md](GEMINI.md) are pointers to this file.

## Where to look

| Need | Read |
|---|---|
| What the plugin does, user-facing overview, debugging/iteration flow, log path (中文段) | [README.md](README.md) |
| Linux (Manjaro) cross-compile → package → log verification SOP | [FOR_AGENT.md](FOR_AGENT.md) |
| Full Windows/PowerShell + bash build commands, `dumpbin` checks, clean rebuild | [BUILD.md](BUILD.md) |
| Link/compile failures already hit before | [PITFALLS.md](PITFALLS.md) |
| CI workflow / GitHub Actions primer | [CI.md](CI.md) / [GITHUB_ACTIONS.md](GITHUB_ACTIONS.md) |
| Branch inventory before creating/deleting branches | [BRANCHES.md](BRANCHES.md) |
| Core modding patterns / CommonLib class index | [MODDING_COOKBOOK.md](MODDING_COOKBOOK.md) / [COMMONLIBSSE_INDEX.md](COMMONLIBSSE_INDEX.md) |
| Chinese-language Skyrim modding knowledge base (Q&A / architecture / tutorials / CommonLibSSE-NG details) — this repo's self-contained copy; the master lives in the parent workspace's `analysis/skyrim_engine/` | [skyrim_mod/INDEX.md](skyrim_mod/INDEX.md) |
| Quest engine design & spec | [QUEST_ENGINE_DESIGN.md](QUEST_ENGINE_DESIGN.md) / [QUEST_ENGINE_SPEC.md](QUEST_ENGINE_SPEC.md) |

## 工作流與鐵律（wf kernel，2026-10-26 非侵入式導入於 `wf/`）

- **要你動手做事** → [WORKFLOWS.md](wf/WORKFLOWS.md) 依意圖派發，再讀該工作流入口檔；進度記 [wf/SESSION-LOG.md](wf/SESSION-LOG.md)，等使用者的記母 repo [WAIT_USER.md](../../WAIT_USER.md)。
- **想看 `wf/` 結構** → [INDEX.md](wf/INDEX.md)；repo 本體的導航仍是上面的「Where to look」表。
- 使用者偏好與邊界 → [user.md](wf/workflows/common/user.md)。

鐵律（always-on）：
1. 重構 / 整理**不改原意**：開發＝行為不變且驗證綠燈（[testing](wf/workflows/testing.md) 的 agent 列）；非開發＝原意不變，照 `Done when:` 驗收。
2. **不可逆或對外的動作**（push、刪除、對外送出）要有**授權來源**：使用者當場確認，或他親自登記的清單項目。都沒有就先問。
3. **條列走資料檔、導航留 md**：給 AI 消化的表／清單 >1 KB 存 `.json`／`.csv`（契約 `wf-table/1`，見 [data-files](wf/workflows/common/data-files.md)），用 `wf/tools/tabledb.py` 讀寫；給人點的導航連結留 md。
4. **具體流程**在各工作流入口檔，不在頂層。

## Project

**DaylightDungeon** — an SKSE plugin for Skyrim SE/AE (incl. 1.6.1170)/GOG/VR, plus the build infrastructure around it (static-CRT CMake/vcpkg presets, Linux clang-cl cross-compile, MO2 packaging, GitHub Actions CI). It started as a fork of the CommonLibSSE-NG template plugin but is no longer a blank template: `src/` carries real features (NpcGenerator, FollowLight, AmbientBoost, hooks). The repo name `my_skyrim_plugin_1` is historical.

Built on [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) (via the `commonlibsse-ng-fork` vcpkg port from the `Monitor221hz/modding-vcpkg-ports` registry — see `vcpkg-configuration.json`). Hook IDs / address-library offsets for each game version still have to be found manually.

The CMake project name (`DaylightDungeon` in `CMakeLists.txt`) becomes the `.dll` filename. `PLUGIN_CONFIG_FOLDER` is a cache-backed CMake value for the subfolder under `SKSE/Plugins/`; CMake deployment and both packaging scripts consume that same value.

## Build

Commands, presets, `dumpbin` checks, clean rebuild, packaging, compiler/CRT/triplet details and install-on-build live in [BUILD.md](BUILD.md); Linux cross-compile in [FOR_AGENT.md](FOR_AGENT.md); past link/compile failures in [PITFALLS.md](PITFALLS.md) (consult before diagnosing a link failure); CI in [CI.md](CI.md) (`.github/workflows/build.yml`), and point users unfamiliar with GitHub Actions at [GITHUB_ACTIONS.md](GITHUB_ACTIONS.md). Hard rules:

- **Static CRT is set in two places** — `CMAKE_MSVC_RUNTIME_LIBRARY` in `CMakePresets.json` and `VCPKG_CRT_LINKAGE static` in `cmake/x64-windows-skse.cmake`. Change both together and wipe `build/`, or the final link dies with LNK2038.
- **Never add `SKSEPluginInfo(...)` in source** — `add_commonlibsse_plugin` in `CMakeLists.txt` generates it (LNK2005 otherwise); plugin metadata is edited in `CMakeLists.txt`.
- **`CMakeLists.txt` does not glob** — register new `.cpp` files in `cmake/sourcelist.cmake` and new headers in `cmake/headerlist.cmake`, or they won't be built.
- On Linux there is no MSVC: use the `build-release-clang-cl-linux` preset. The Windows flow is PowerShell — no bash `&&` chaining.
- No C++ test suite; packaging / quest-PRF contract tests are listed in [testing](wf/workflows/testing.md).

## Non-build directories

- `research/` — feasibility analyses, every engine-level claim checked against the CommonLibSSE-NG `RE/` headers. **Read before designing a new feature** — several dead ends (dynamic navmesh generation, runtime native-dialogue injection) are already written up there.
- [`archive/`](archive/README.md) — dated snapshots of stopped projects and superseded docs. **Not current state**; the index there says what each one is and why it was retired.
- `vendor/` — implementations lifted off old feature branches, kept as reference. Deliberately **not** in `sourcelist.cmake`, so nothing here compiles. `vendor/README.md` records which ones no longer build against current CommonLibSSE-NG and exactly which APIs moved.
- `BRANCHES.md` — per-branch inventory: what each contains, whether it actually cross-compiles, and the SHA to recover it. Consult before creating or deleting branches.

## Code layout

Per-file responsibilities of `src/` and the `util.h` helper index live in [code-map](wf/workflows/common/code-map.md). When adding utilities, extend the `util.h` helpers instead of inventing parallel ones.

## Debugging

Attach a debugger only with a legal Skyrim copy whose `.exe` has been stripped with Steamless. MO2 users need `-forcesteamloader` as an SKSE argument so plugins load with the stub removed. A minimal VS Code `launch.json` (`cppvsdbg`, `request: attach`) example is in `README.md`.

Under Proton, attaching is impractical — rely on the trace log instead: `<Proton-prefix>/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/DaylightDungeon.log` (every level flushes immediately, see `src/log.h`).

<!-- wf-kernel v0.6+368468a (2026-10-26 導入) -->
