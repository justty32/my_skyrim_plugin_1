# Build

## TL;DR

```powershell
cmake --preset build-release-msvc; if ($?) { cmake --build build/release-msvc }
```

產出：`build/release-msvc/DaylightDungeon.dll`。

## 先決條件

- Windows 10/11 + Visual Studio 2022（裝 "Desktop development with C++" workload，MSVC toolset + Windows SDK + CMake + Ninja 會一起進來）。
- [vcpkg](https://github.com/microsoft/vcpkg) clone 下來、跑過 `bootstrap-vcpkg.bat`，環境變數 `VCPKG_ROOT` 指向 clone 資料夾。
- 可選：在「Developer PowerShell for VS 2022」或一般 pwsh 裡操作（一般 pwsh 只要 CMake / Ninja 能在 `PATH` 上找到就行；`dumpbin` 則只有 Developer PowerShell 找得到）。

## Presets

`CMakePresets.json` 定義了兩組：

| Configure preset      | CMAKE_BUILD_TYPE | Binary dir             |
| --------------------- | ---------------- | ---------------------- |
| `build-debug-msvc`    | Debug            | `build/debug-msvc/`    |
| `build-release-msvc`  | Release          | `build/release-msvc/`  |

兩組都用 MSVC + Ninja + 自定義 triplet `x64-windows-skse`（overlay 在 `cmake/x64-windows-skse.cmake`），靜態連結 MSVC runtime（`/MT` release、`/MTd` debug），所以產出的 `.dll` 不依賴 `vcredist`。

## PowerShell（Windows）

Configure + build（Release）：

```powershell
cmake --preset build-release-msvc
if ($?) { cmake --build build/release-msvc }
```

或一行版：

```powershell
cmake --preset build-release-msvc; if ($?) { cmake --build build/release-msvc }
```

Debug 同樣的形式，把 `release-msvc` 換成 `debug-msvc`。

指定平行度：

```powershell
cmake --build build/release-msvc -j 8
```

Clean build（當 `vcpkg.json` / `vcpkg-configuration.json` / triplet overlay 改過，必須砍掉 `build/` 才能讓 vcpkg 重建所有 deps，不然會拿到舊 CRT 的 cache 結果 LNK2038 mismatch）：

```powershell
Remove-Item -Recurse -Force build
cmake --preset build-release-msvc
cmake --build build/release-msvc
```

## bash（Git Bash on Windows / MSYS2）

同樣的 cmake 指令，只是 shell 語法不同：

```bash
cmake --preset build-release-msvc && cmake --build build/release-msvc
rm -rf build
```

> 直接在 Manjaro 上跑這些 MSVC preset 不會成功，因為那邊沒有 MSVC。Linux 上要改用已架好的 cross-compile preset `build-release-clang-cl-linux`（clang-cl + lld-link + xwin，toolchain 在 `cmake/toolchain-clang-cl-xwin.cmake`），完整指令見 [FOR_AGENT.md](FOR_AGENT.md)。

## VS Code 工作流

VS Code 的 CMake Tools 套件開資料夾時會自動跑 `cmake --preset <你選的>` 去產生 `build/<preset>/`，然後 `F7` 或狀態列的 Build 按鈕相當於 `cmake --build build/<preset>`。上面的 CLI 指令是用在：腳本化、CI、或想手動做 clean rebuild 時。

## 驗證 DLL 真的是 standalone

Build 完後，檢查 DLL 的 import 表，確認沒有任何 MSVC runtime DLL 在裡面：

```powershell
# 需要 Developer PowerShell for VS 2022（dumpbin 才在 PATH 上）
dumpbin /dependents build\release-msvc\DaylightDungeon.dll
```

預期看到的只有 Windows 系統 DLL：`KERNEL32.dll`、`USER32.dll`、`d3d11.dll` 之類。

**不該出現**：`MSVCP140.dll`、`VCRUNTIME140.dll`、`VCRUNTIME140_1.dll`、`MSVCP140D.dll`、`VCRUNTIME140D.dll`。若出現任何一個，代表某個 vcpkg dep 還是用 dynamic CRT 編的（通常是 build cache 沒清乾淨）——砍 `build/` 重來。

## 產出位置

- Release：`build/release-msvc/DaylightDungeon.dll`
- Debug：`build/debug-msvc/DaylightDungeon.dll` + `DaylightDungeon.pdb`

沒設 `SKYRIM_FOLDER` / `SKYRIM_MODS_FOLDER` 時不會自動安裝。要丟進 Manjaro 的 MO2，用 `scripts/pack.ps1` 打包：

```powershell
scripts\pack.ps1                      # Release，產出 dist\DaylightDungeon-0.0.1.zip
scripts\pack.ps1 -Config debug-msvc   # Debug，產出 dist\DaylightDungeon-0.0.1-Debug.zip
```

腳本會從 `build/<config>/CMakeCache.txt` 讀出 plugin 名稱、版本與 `PLUGIN_CONFIG_FOLDER`，在 `pack/` 暫存出標準 MO2 結構（`Data/SKSE/Plugins/DaylightDungeon.dll` + `Data/SKSE/Plugins/DaylightDungeon/`），最後壓成 `dist/<PluginName>-<version>[-Debug].zip`。`OutputDir` 不可指向 `pack/` 或其子目錄。產出的 zip 直接拖進 MO2 的「Install a new mod from an archive」即可。

不需完整 toolchain 的打包契約測試：

```powershell
scripts\test_packaging.ps1
```

## 編譯設定細節（2026-10-26 自 AGENTS.md「Build」「Adding source files」段移入，原文照搬）

PowerShell 不用 bash 的 `&&` 串接（見上方 PowerShell 段的寫法）。

Requires MSVC (VS 2022 with the "Desktop development with C++" workload), Ninja, and vcpkg with `VCPKG_ROOT` set. Presets live in `CMakePresets.json`. On Linux there is no MSVC — use the `build-release-clang-cl-linux` preset per `FOR_AGENT.md`. `scripts/pack.ps1` (Windows) / `scripts/pack.sh` (Linux) build the MO2-installable zip into `dist/`.

- Configure presets: `build-debug-msvc`, `build-release-msvc`. Build presets of the same name minus the `build-` prefix (`debug-msvc`, `release-msvc`) exist for `cmake --build --preset ...`.
- Binary dirs: `build/debug-msvc`, `build/release-msvc`.
- C++23, MSVC with `/permissive- /Zc:preprocessor /EHsc /MP`, **static CRT** (`/MT` release, `/MTd` debug — `CMAKE_MSVC_RUNTIME_LIBRARY` in `CMakePresets.json`, `VCPKG_CRT_LINKAGE static` in `cmake/x64-windows-skse.cmake`). Everything links statically into the .dll so it runs on Manjaro/Proton without needing vcredist installed in the prefix. If you change either of those knobs you must change both to match, and wipe `build/` so vcpkg rebuilds all deps against the new CRT — otherwise the final link dies with LNK2038 runtime library mismatch.
- Triplet `x64-windows-skse` (overlay in `cmake/x64-windows-skse.cmake`); library linkage is static for non-SKSE ports and dynamic for ports matching `fully-dynamic-game-engine|skse|qt*`.
- `SKSEPluginInfo` is generated by `add_commonlibsse_plugin` in `CMakeLists.txt` — never add `SKSEPluginInfo(...)` in source (LNK2005 duplicate symbol). Plugin metadata (author, version, …) is edited in `CMakeLists.txt`.
- `src/PCH.h` is the required precompiled header — it includes `RE/Skyrim.h` and `SKSE/SKSE.h` and enables `using namespace std::literals`.
- No C++ test suite is configured (`testPresets` in `CMakePresets.json` is empty). `scripts/test_packaging.ps1` (Windows, tests `pack.ps1`) and `scripts/test_packaging.sh` (Linux, tests `pack.sh`) are toolchain-free regression tests for the packaging and release-name contract — the two pack scripts are independent, so run the one matching the script you touched. `scripts/test_quest_prf.{ps1,sh}` covers the quest-PRF contract.

### Install-on-build

Post-build steps in `CMakeLists.txt` copy the `.dll` (and `.pdb` in Debug) into `SKSE/Plugins/` under whichever output root is defined:

- `SKYRIM_FOLDER` env var → `<SKYRIM_FOLDER>/Data/SKSE/Plugins/`
- `SKYRIM_MODS_FOLDER` env var (MO2/Vortex) → `<SKYRIM_MODS_FOLDER>/<ProjectName> <BuildType>/SKSE/Plugins/`

If a `config/` folder exists at the repo root, its contents are also copied to `SKSE/Plugins/<PLUGIN_CONFIG_FOLDER>/` (currently `DaylightDungeon`; today it holds `FollowLight.ini` plus `schema/`). If neither env var is set, the DLL just stays in the build dir.

### Adding source files

`CMakeLists.txt` does not glob. New `.cpp` files must be registered in `cmake/sourcelist.cmake` and new headers in `cmake/headerlist.cmake`, or they won't be built.
