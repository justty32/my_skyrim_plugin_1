# Build

## TL;DR

```powershell
cmake --preset build-release-msvc; if ($?) { cmake --build build/release-msvc }
```

產出：`build/release-msvc/TemplatePlugin.dll`。

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

## PowerShell（Windows，目前開發環境）

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

## bash（Git Bash on Windows / MSYS2 / 未來 cross-compile）

同樣的 cmake 指令，只是 shell 語法不同：

```bash
cmake --preset build-release-msvc && cmake --build build/release-msvc
rm -rf build
```

> 直接在 Manjaro 上跑這些不會成功，因為那邊沒有 MSVC。從 Linux 產出 Windows `.dll` 需要另外架 toolchain（clang-cl + wine，或 MinGW-w64 + LLVM），屬於路線圖的 step 2，還沒做。

## VS Code 工作流（你現在的方式）

VS Code 的 CMake Tools 套件開資料夾時會自動跑 `cmake --preset <你選的>` 去產生 `build/<preset>/`，然後 `F7` 或狀態列的 Build 按鈕相當於 `cmake --build build/<preset>`。上面的 CLI 指令是用在：腳本化、CI、或想手動做 clean rebuild 時。

## 驗證 DLL 真的是 standalone

Build 完後，檢查 DLL 的 import 表，確認沒有任何 MSVC runtime DLL 在裡面：

```powershell
# 需要 Developer PowerShell for VS 2022（dumpbin 才在 PATH 上）
dumpbin /dependents build\release-msvc\TemplatePlugin.dll
```

預期看到的只有 Windows 系統 DLL：`KERNEL32.dll`、`USER32.dll`、`d3d11.dll` 之類。

**不該出現**：`MSVCP140.dll`、`VCRUNTIME140.dll`、`VCRUNTIME140_1.dll`、`MSVCP140D.dll`、`VCRUNTIME140D.dll`。若出現任何一個，代表某個 vcpkg dep 還是用 dynamic CRT 編的（通常是 build cache 沒清乾淨）——砍 `build/` 重來。

## 產出位置

- Release：`build/release-msvc/TemplatePlugin.dll`
- Debug：`build/debug-msvc/TemplatePlugin.dll` + `TemplatePlugin.pdb`

沒有自動安裝步驟（因為你沒設 `SKYRIM_FOLDER` / `SKYRIM_MODS_FOLDER`）。要丟進 Manjaro 的 MO2，用 `scripts/pack.ps1` 打包：

```powershell
scripts\pack.ps1                      # Release，產出 dist\TemplatePlugin-0.0.1.zip
scripts\pack.ps1 -Config debug-msvc   # Debug，產出 dist\TemplatePlugin-0.0.1-Debug.zip
```

腳本會從 `build/<config>/CMakeCache.txt` 讀出 plugin 名稱與版本，在 `pack/` 暫存出標準 MO2 結構（`Data/SKSE/Plugins/<PluginName>.dll` + 若 `config/` 存在則一併複製成 `Data/SKSE/Plugins/Template_Plugin/`），最後壓成 `dist/<PluginName>-<version>[-Debug].zip`。產出的 zip 直接拖進 MO2 的「Install a new mod from an archive」就行。
