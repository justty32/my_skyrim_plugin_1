# TemplatePlugin

An SKSE plugin for Skyrim SE / AE (incl. 1.6.1170) / GOG / VR, built on [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG). Forked from [Monitor221hz/CommonLibSSE-NG-Template-Plugin](https://github.com/Monitor221hz/CommonLibSSE-NG-Template-Plugin) with extra build infrastructure added on top — see below.

> Rename `TemplatePlugin` in `CMakeLists.txt` (`project(...)`) and the `CONFIG_FOLDER` string near line 60 when using this as a starting point for a real plugin; also update the two hardcoded paths in `.github/workflows/build.yml` and `$ConfigFolderName` in `scripts/pack.ps1`.

## What this adds on top of the upstream template

- **Static CRT build** — produced `.dll` has no dependency on `MSVCP140.dll` / `VCRUNTIME140.dll`; runs on Proton / Wine prefixes without `vcredist`
- **MO2 packaging script** — `scripts/pack.ps1` assembles a `Data/SKSE/Plugins/...` zip ready to install via MO2's archive importer
- **CI (GitHub Actions)** — every push builds on `windows-latest`, caches vcpkg port binaries, verifies the static-CRT build via `dumpbin`, and uploads the `.dll` + MO2 zip as artifacts. Free on public repos.
- **Lifecycle logging** — `MessageHandler` in `src/plugin.cpp` logs every SKSE message type, including player name on save load
- **Documentation**:
  - [`BUILD.md`](BUILD.md) — PowerShell / bash build commands, clean-rebuild rules, `dumpbin` verification
  - [`PITFALLS.md`](PITFALLS.md) — traps this project has hit (LNK2005 from duplicate `SKSEPluginInfo`, LNK2038 from mismatched CRT, silent link failures, `<tuple>` not found, ...)
  - [`CI.md`](CI.md) — how the GitHub Actions workflow is put together; cache key strategy; what to change when renaming
  - [`GITHUB_ACTIONS.md`](GITHUB_ACTIONS.md) — primer on Actions concepts (workflow / job / step / runner / action / artifact / cache), 中文

## Requirements

- Windows 10/11 with Visual Studio 2022 (Community edition works) — "Desktop development with C++" workload
- [`vcpkg`](https://github.com/microsoft/vcpkg) bootstrapped, with `VCPKG_ROOT` environment variable pointing at the clone
- (Optional) `SKYRIM_FOLDER` or `SKYRIM_MODS_FOLDER` env var if you want the post-build step to drop the `.dll` directly into the game folder / a mod manager's mods folder

## Quick start

```powershell
# Configure + build (Release)
cmake --preset build-release-msvc
cmake --build build/release-msvc

# Package into MO2-installable zip
scripts\pack.ps1
```

Full build instructions (PowerShell / bash / VS Code) in [`BUILD.md`](BUILD.md).

## Debugging

A debugger can only attach to Skyrim.exe that's been stripped of its Steam DRM using Steamless. MO2 users need `-forcesteamloader` in SKSE arguments. Minimal VS Code `launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "C++ Debugger",
            "type": "cppvsdbg",
            "request": "attach"
        }
    ]
}
```

For runtime on Proton (Skyrim under Linux / Steam Deck), native debugger attach isn't practical; read the plugin log instead:

```
<prefix>/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/<PluginName>.log
```

(Where `<prefix>` is typically `~/.steam/steam/steamapps/compatdata/489830/pfx/`.)

## Acknowledgments

- [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) by CharmedBaryon — the Skyrim RE library this whole ecosystem runs on
- [CommonLibSSE-NG-Template-Plugin](https://github.com/Monitor221hz/CommonLibSSE-NG-Template-Plugin) by Monitor221hz — the starting-point template this project forked from
- [`commonlibsse-ng-fork`](https://github.com/Monitor221hz/modding-vcpkg-ports) vcpkg port, also by Monitor221hz

---

## 開發與測試流程（中文）

本專案在 Manjaro Linux 上開發，但目標是 Windows `.dll`，沒辦法本地 build；iteration 全走 GitHub Actions。實機跑在 Proton / Wine 底下，難以 attach debugger，所以**所有 debug 都靠 log 檔**。

### GitHub Actions 觸發條件

`.github/workflows/build.yml` 的 `on:` 設定：

- push 到 `main`
- 任何 PR（含 fork PR）
- 手動 `workflow_dispatch`

**Push 到 feature branch 不會自動 build**。要讓 feature branch 的 commit 跑 CI，以下擇一：

- 在 GitHub 上開 PR 到 `main`（推薦，會觸發 CI 並附 checks 狀態）
- 到 `Actions` tab → `Build` → `Run workflow` → 選 branch 手動觸發

### 一次完整 iteration

```
                 Manjaro (寫 code)
                        │
                   git push feature
                        │
                開 PR 或 workflow_dispatch
                        │
                  GitHub Actions
             (cold ~20 min, cached ~2–3 min)
                        │
       下載 artifact（TemplatePlugin-mo2-zip）
                        │
        MO2 → Install a new mod from an archive
                        │
                   啟動 Skyrim（Proton）
                        │
             讀取 log 檔、觀察遊戲內行為
                        │
               把 log / 現象回報 assistant
                        │
                  修 code，回到第一步
```

### Log 檔路徑

插件名為 `<PluginName>`（目前 `TemplatePlugin`，正式用要在 `CMakeLists.txt` 改）。log 位於 Proton prefix 內：

```
<prefix>/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/<PluginName>.log
```

典型 `<prefix>` 為 `~/.steam/steam/steamapps/compatdata/489830/pfx/`（489830 是 Skyrim SE 的 Steam AppID）。

### Log 層級

`src/log.h::SetupLog()` 將 spdlog level 強制設為 `trace` 且 `flush_on(trace)`，所以即使 Release build，所有 `SKSE::log::trace/debug/info/warn/error` 都會立即寫進 log 檔。目前 CI 只 build Release（`build.yml` 沒做 Debug matrix）。若日後要讓 Release 變安靜，用 `#ifndef NDEBUG` gate 掉 verbose 區段。

### 程式風格：多 log

因為 debug 只能靠 log，所有 handler / hook / callback / form 建立步驟都要有進入點與出口的 log，以及中間關鍵狀態（pointer 有無、formID、enum 值、數值）。原則：

- 每個主流程前後各一條 `info` 標記範圍
- 每個 pointer lookup 的結果寫 `info`（nullability 會回報）
- 每一次 factory `Create()` 的結果寫 `trace`（太多可壓 debug）
- 失敗條件（null、lookup fail、out-of-range）寫 `warn` 或 `error`

### 第一次 CI build compile error 怎麼辦

假設 CI fail 在 `Configure` 或 `Build` step：

1. 開該次 run，展開失敗的 step
2. 找第一行 `error:` 或 `error C\d+:`（MSVC 常把一個錯誤後面吐一連串 cascade，看最上面那條）
3. 把該段（前後各約 20 行）貼給 assistant 即可快修

常見第一次就撞的錯：

- 名稱不對：`RE::XXX` 或 `kYyy` 拼錯 → 名稱查 `build/release-msvc/vcpkg_installed/x64-windows-skse/include/RE/...` 頭檔
- `Advapi32.lib` 打不開 → 其實是 LNK2005 cascade，回看本 run 上方 LNK2005 的真正重定義
- LNK2038 runtime library mismatch → `PITFALLS.md` 第 2 條
- `.exp` / `.lib` 有但 `.dll` 沒產生 → link 階段噴硬錯（`PITFALLS.md` 第 3 條）

### 動態建立 Form 的已知風險（對應 `skyrim_mod/tutorial/Systems_20_Dynamic_Form_Creation.md`）

純 C++ 插件（無 ESP）要給玩家「一把武器」「一個法術」「一段龍吼」時，會走 `IFormFactory::GetConcreteFormFactoryByType<T>()->Create()` 動態造 form。注意：

1. **Save bloat**：動態建立的 form（通常 `0xFF` 開頭）會被永久寫進存檔。要在每次 `kDataLoaded` 建一次（plugin 生命週期一次），而不是每次讀檔都建，否則存檔會越來越腫。正確流程：
   - 用一組 `static` 指標快取已建立的 form
   - 若要跨遊戲重啟保留 formID 的對應關係，走 SKSE Co-Save / SKSE Serialization
2. **首次不確定性**：越複雜的 form（龍吼 > 法術 > 武器），需要設的欄位越多、越可能漏（MenuIcon、Voice line、Animation event 等），實機可能 compile 過但 runtime 不動。策略是第一輪把最少必要欄位設齊 + 多 log，拿 log 決定下一步修哪裡，不要一次塞滿所有 `data.*` 欄位。
3. **沒有 3D 模型就沒法憑空造**：動態 form 只能指向既有的 `.nif` 路徑（原版或已安裝的其他 mod），不能在代碼裡「生」出一把新造型的武器。

### 當前分支

| Branch | 功能 | 狀態 |
| --- | --- | --- |
| `feat/power-shout` | 動態 3 段龍吼「力量強化」，每段 +100/+200/+300 HP·SP·MP 自身 buff 60s | 尚未驗證 CI（需開 PR 或 `workflow_dispatch`） |

