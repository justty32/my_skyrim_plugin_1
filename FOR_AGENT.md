# Agent Workflow Guide (Manjaro Linux Development)

本文件定義了在此環境（Manjaro Linux 交叉編譯）開發、編譯、打包與驗證 SKSE 插件的標準作業程序。

## 1. 環境上下文 (Environment Context)
- **開發主機 (Host):** Manjaro Linux
- **目標系統 (Target):** Windows x64
- **編譯器:** `clang-cl` (透過 `xwin` toolchain)
- **遊戲運行:** Steam/Proton (AppID 489830)
- **API 參考:** `./CommonLibSSE-NG/` (最權威的標頭檔與代碼參考)
- **輔助資料:** `./skyrim_mod/` (包含解析資料，僅供參考，需經實測驗證)

## 2. 開發規範 (Development Conventions)
- **檔案註冊:** 
  - 新增 `.cpp` 檔案務必加入 `cmake/sourcelist.cmake`。
  - 新增 `.h` 檔案務必加入 `cmake/headerlist.cmake`。
  - **不可以使用** CMake globbing 自動搜尋檔案。
- **SKSE 插件資訊:**
  - 不要手動在代碼中使用 `SKSEPluginInfo(...)`。
  - 資訊由 `CMakeLists.txt` 中的 `add_commonlibsse_plugin` 自動產生。
- **除錯紀律:**
  - 由於在 Proton 下難以 attach debugger，**所有除錯皆依賴 log 檔**。
  - 關鍵步驟、Pointer 檢查、Form 建立成功與否，都必須寫入 log (`SKSE::log::info/trace`)。

## 3. 指令集 (Command Palette)

### 3.1 配置 (Configure)
若 `vcpkg.json` 或 Triplet 有變動，或編譯報錯建議重新配置時：
```bash
cmake --preset build-release-clang-cl-linux
```

### 3.2 編譯 (Build)
```bash
cmake --build build/release-clang-cl-linux -j$(nproc)
```

### 3.3 打包 (Package)
編譯成功後，產出可供 MO2 安裝的 `.zip` 檔：
```bash
./scripts/pack_env.sh --config release-clang-cl-linux
```
打包後的檔案通常位於 `dist/` 目錄。

## 4. 驗證與日誌 (Verification & Logs)

### 4.1 日誌路徑
SKSE 日誌位於 Proton Prefix 內：
```bash
/home/lorkhan/.local/share/Steam/steamapps/compatdata/489830/pfx/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/TemplatePlugin.log
```

### 4.2 驗證指令
檢查日誌更新狀態與內容：
```bash
ls -l "/home/lorkhan/.local/share/Steam/steamapps/compatdata/489830/pfx/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/TemplatePlugin.log"
tail -n 50 "/home/lorkhan/.local/share/Steam/steamapps/compatdata/489830/pfx/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/TemplatePlugin.log"
```

## 5. 成功標準 (Success Criteria)
1. `cmake --build` 順利完成無 error。
2. `pack_env.sh` 在 `dist/` 產生新的 `.zip`。
3. 遊戲啟動後，Log 檔的修改時間有更新。
4. Log 內容無 `[error]` 或 `[critical]` 標籤。
