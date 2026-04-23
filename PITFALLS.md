# 踩坑記錄

此專案在 Windows 上用 vcpkg + MSVC 編 CommonLibSSE-NG SKSE plugin 時踩過的雷。下次遇到類似症狀直接對照。

---

## 1. `add_commonlibsse_plugin()` 自動產生 `SKSEPluginInfo`，不要再寫第二份

### 症狀

```
__TemplatePluginPlugin.cpp.obj : error LNK2005: SKSEPlugin_Query already defined in plugin.cpp.obj
__TemplatePluginPlugin.cpp.obj : error LNK2005: SKSEPlugin_Version already defined in plugin.cpp.obj
LINK : fatal error LNK1181: cannot open input file 'Advapi32.lib'
ninja: build stopped: subcommand failed.
```

### 原因

`CMakeLists.txt` 裡的 `add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ...)` 在 configure 階段會在 binary dir 自動產出一份檔案 `__<TARGET>Plugin.cpp`（例如 `build/release-msvc/__TemplatePluginPlugin.cpp`），內容大致是：

```cpp
#include "REL/Relocation.h"
#include "SKSE/SKSE.h"

SKSEPluginInfo(
    .Version = REL::Version{ 0, 0, 1, 0 },
    .Name = "TemplatePlugin"sv,
    .Author = ""sv,
    .SupportEmail = ""sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
    .MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }
)
```

`SKSEPluginInfo` macro 一展開就定義了兩個 exported 符號：`SKSEPlugin_Version`（`SKSE::PluginDeclaration` 實體）和 `SKSEPlugin_Query`（函式）。如果你在 `plugin.cpp` 再寫一份 `SKSEPluginInfo(...)`，兩處都定義這兩個符號 → LNK2005 重複定義。

### 那個 LNK1181 `Advapi32.lib` 呢？

不是真的打不開 Advapi32.lib。MSVC linker 撞到 LNK2005 重定義後會進入 error-recovery，連帶把後面標準 lib 名稱吐 LNK1181。**是 cascade，不是獨立問題**，修掉 LNK2005 它自己就消失。

### 修法

- **不要**在 user source 裡加 `SKSEPluginInfo(...)`。
- 要改 plugin 資訊（author、email、版本、最小 SKSE 版本、相容 runtime 等），在 `CMakeLists.txt` 裡傳參數給 `add_commonlibsse_plugin()`。所有支援的參數看 `build/<preset>/vcpkg_installed/x64-windows-skse/share/CommonLibSSE/CommonLibSSE.cmake`（`commonlibsse-ng-fork` 裝好後一定在這個路徑），核心的：
  ```cmake
  add_commonlibsse_plugin(${PROJECT_NAME}
      AUTHOR "Your Name"
      EMAIL "you@example.com"
      SOURCES ${headers} ${sources})
  ```
  可用參數：
  - 旗標：`USE_ADDRESS_LIBRARY`、`USE_SIGNATURE_SCANNING`、`STRUCT_DEPENDENT`、`EXCLUDE_FROM_ALL`、`OPTIONAL`
  - 單值：`NAME`、`AUTHOR`、`EMAIL`、`VERSION`、`MINIMUM_SKSE_VERSION`
  - 多值：`COMPATIBLE_RUNTIMES`（最多 16 個版本號；和 `USE_ADDRESS_LIBRARY` / `USE_SIGNATURE_SCANNING` 互斥）、`SOURCES`
- 預設行為已經是 `USE_ADDRESS_LIBRARY=TRUE` + `StructCompatibility::Independent`，所以 AE 1.6.1170 本來就涵蓋，**不需要額外宣告**。
- **沒有** opt-out 旗標可以關掉自動產生 `__${TARGET}Plugin.cpp` 這個檔，所以「我想完全自己寫 `SKSEPluginInfo`」這條路在這個 fork 目前走不通。

### 驗證

Build 完後檢查 `build/<preset>/__<TARGET>Plugin.cpp` 這個檔存在且內容是自動產的 `SKSEPluginInfo`。

---

## 2. 切換 static CRT 要同時改兩個地方，並砍掉 `build/`

### 症狀

Link 階段噴：
```
error LNK2038: mismatch detected for 'RuntimeLibrary':
    value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease' in <some>.obj
```
（或反過來：`MD_DynamicRelease` 對上 `MT_StaticRelease`。）

### 原因

靜態 / 動態 C runtime 必須**整條 link chain 一致**。兩個地方決定 CRT linkage：

1. **vcpkg 編的 deps**（CommonLibSSE、spdlog、fmt、DirectXTK 等）→ 由 **triplet** 決定，本專案是 `cmake/x64-windows-skse.cmake` 的 `VCPKG_CRT_LINKAGE`。
2. **自己 plugin 的 .obj** → 由 **CMake 的 `CMAKE_MSVC_RUNTIME_LIBRARY`** 決定（`CMakePresets.json` 裡設的）。

兩者不一致，link 時 MSVC 就會比對每個 .obj 的 `/MT` vs `/MD` 標記、炸 LNK2038。

此外：vcpkg 會把編好的 deps cache 在 `build/<preset>/vcpkg_installed/`。triplet 改了但 cache 沒清，會繼續用舊 CRT 的 `.lib`。

### 修法

兩處同時改，對應關係：

| 你想要的 | `x64-windows-skse.cmake` 的 `VCPKG_CRT_LINKAGE` | `CMAKE_MSVC_RUNTIME_LIBRARY` |
| --- | --- | --- |
| Static (`/MT` / `/MTd`) | `static` | `MultiThreaded$<$<CONFIG:Debug>:Debug>` |
| Dynamic (`/MD` / `/MDd`) | `dynamic` | `MultiThreaded$<$<CONFIG:Debug>:Debug>DLL` |

然後**必須砍 `build/`**：

```powershell
Remove-Item -Recurse -Force build
```

讓 vcpkg 把所有 deps 用新 CRT 從頭重編。vcpkg 會把每個 port 做過的 build 結果 cache 在 `%VCPKG_ROOT%\downloads` 與 `%VCPKG_ROOT%\buildtrees`，跟 `build/` 本地的 `vcpkg_installed/` 是兩層 cache，通常砍 `build/` 就夠；若還是拿到舊結果，再去 `vcpkg_installed` 和 vcpkg 全域 cache 清。

### 驗證

（Developer PowerShell for VS 2022 才有 `dumpbin`）

```powershell
dumpbin /dependents build\release-msvc\TemplatePlugin.dll
```

DLL 的 imports 不該出現：`MSVCP140.dll`、`VCRUNTIME140.dll`、`VCRUNTIME140_1.dll`、`MSVCP140D.dll`、`VCRUNTIME140D.dll`。只看到 `KERNEL32.dll`、`USER32.dll`、`d3d11.dll` 這類 Windows 內建 DLL 就對了。

Proton / Wine 在 lutris-gaming 之類的 prefix 裡通常有裝 vcredist，但不是必然；靜態 CRT 直接繞開這個潛在問題。

---

## 3. 只看到 `.exp` 和 `.lib` 但沒有 `.dll` = link 失敗

### 症狀

`build/<preset>/` 下有 `TemplatePlugin.exp`、`TemplatePlugin.lib`，**沒有 `TemplatePlugin.dll`**。VS Code 的 build 面板可能還顯示成功或錯誤不明顯。

### 原因

MSVC link DLL 的兩階段流程：

1. **第一階段**：讀每個 `.obj` 的 export table → 產出 `.exp`（export metadata）和 `.lib`（import library）。
2. **第二階段**：做真正的 link → 產出 `.dll`。

如果第二階段噴 LNK2005 / LNK2019 / LNK1181 之類的硬錯誤，`.exp` / `.lib` 會留在磁碟上（它們是一階段的產物），但 `.dll` 不會被寫出來。檔案總管看起來就像「只產了 `.lib`」，容易誤以為 build 是成功但 target 類型錯了（以為變成 static library），其實根本沒 link 完。

### 修法

命令列重跑 `cmake --build build/<preset>`（**要在 Developer PowerShell for VS 2022 裡跑**，見第 4 條），看完整 stderr。VS Code 的 Output 面板也有，選「CMake/Build」channel。

### 補充：import library（`.lib`）≠ static library

- **Import library**：`.dll` 的「目錄」，幾 KB 大小，記錄該 DLL 匯出了哪些 symbol。別的 DLL / exe 要 call 這個 DLL 的函式時，link 階段吃這個 `.lib` 填 import 表。
- **Static library**：真正的 object code 集合，通常幾 MB，包含函式實作。

MSVC 只要你的 DLL 有 `__declspec(dllexport)` 符號（SKSE plugin 必然有）就會同時吐出 import library，這是**正常行為**。SKSE plugin 是葉節點（SKSE 用 `GetProcAddress` 動態呼叫，沒人會 static-link 它），所以這個 `.lib` 你永遠用不到，無視即可。

---

## 4. 命令列 build 噴 `fatal error C1083: Cannot open include file: 'tuple'`

### 症狀

VS Code 裡 build 能過，自己開一般 PowerShell / bash 跑 `cmake --build build/<preset>` 就炸：

```
fatal error C1083: Cannot open include file: 'tuple': No such file or directory
```

### 原因

`cl.exe` 找 header 的路徑不是寫死在編譯器裡的，是透過 `INCLUDE` 環境變數告知。同樣地，link 階段找 `.lib` 靠 `LIB` 環境變數。這兩個變數平常不存在，要靠下列其中一種方式設好：

- `vcvarsall.bat x64` 手動呼叫（cmd 或 wrapper）
- 從「**Developer Command Prompt / Developer PowerShell for VS 2022**」開 session（這兩個 shortcut 開起來就已經設好了，開始功能表搜尋看得到）
- VS Code 的 CMake Tools extension 在 configure/build 時會自動幫你走 vcvars

一般 PowerShell / Git Bash / MSYS2 等「乾淨」shell 裡**沒有**這些變數，所以直接跑 `cl.exe` 連 `<tuple>` 都找不到。

### 修法

**命令列 build 走 Developer PowerShell for VS 2022**。開始功能表搜「Developer PowerShell」就有。

一般 PowerShell 要加載 VS dev env 的 wrapper 範例：

```powershell
# 一次性：讓當前 pwsh session 吃到 vcvars
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
& "${env:COMSPEC}" /c "`"$vsPath\VC\Auxiliary\Build\vcvars64.bat`" && set" |
  ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)') { Set-Item "env:$($Matches[1])" $Matches[2] }
  }
```

之後同一個 session 裡 `cmake --build` 就能用。VS Code 裡 build 沒這個問題。

---

## 5. SKSE plugin 必須放在 `Data/SKSE/Plugins/`，不是 `Data/SKSE/`

SKSE 啟動時掃的是 `Data/SKSE/Plugins/*.dll`，**不會**掃 `Data/SKSE/` 根目錄。MO2 zip 打包時要確保路徑是 `Data/SKSE/Plugins/<PluginName>.dll`，否則 SKSE 根本不會看到你的 plugin、`.log` 也不會產生。

---

## 6. 「port」是什麼（常識備忘）

vcpkg 的 **package recipe**，跟網路 port 無關。一個 port = 一個資料夾，名字就是函式庫名，裡面至少有 `portfile.cmake`（build 腳本）和 `vcpkg.json`（manifest）。

本專案用兩個 registry（見 `vcpkg-configuration.json`）：

- `microsoft/vcpkg` — 官方 registry，提供 `simpleini`、`nlohmann-json`、`directxtk` 等通用 port。
- `Monitor221hz/modding-vcpkg-ports` — 社群 registry，提供 `commonlibsse-ng-fork`（官方沒有，是 SKSE 社群 fork）。

---

## 通用紀律

**Link error 一定從第一條看起。** MSVC linker 撞到硬錯後常噴一連串看似無關的二階錯誤（例如 `Advapi32.lib` 打不開），但根本原因可能只是最上面那個 LNK2005 / LNK2019。修最上面那條、重 link，下面通常就一起消失。不要從中間挑一條開始猜。
