# CommonLibSSE NG 目錄結構 (01_structure)

CommonLibSSE NG 的代碼組織非常清晰，主要分為頭文件（include）和實現文件（src）。

## 核心目錄佈局

### 1. `include/` (頭文件)
這是開發者最常接觸的部分，包含了所有類和函數的定義。
- **`RE/` (Reverse Engineering)**:
  這是最核心的部分，包含了對 Skyrim 引擎內部類的定義。它按字母順序排列，幾乎涵蓋了遊戲的所有系統（如：渲染、腳本、AI、UI 等）。
- **`REL/` (Relocation)**:
  包含用於處理內存重定位的類，特別是與 Address Library 相關的邏輯。它允許通過 ID 查找函數地址。
- **`REX/` (Extensions)**:
  包含一些擴展功能，通常是與特定版本或特定平台相關的增強功能。
- **`SKSE/`**:
  包含了與 SKSE 本身交互的封裝，如 Log（日誌）、Trampoline（蹦床/掛鉤）、Interfaces（接口）等。

### 2. `src/` (源文件)
包含了對應頭文件的實現代碼。
- **`RE/`**: 引擎類函數的具體實現。
- **`REL/`**: 地址查找和重定位的實現。
- **`SKSE/`**: SKSE 接口的包裝實現。

### 3. `cmake/`
包含了構建系統所需的 CMake 腳本，如定義如何查找依賴項、如何設置編譯選項等。

### 4. `scripts/`
包含了一些實用的輔助腳本，例如用於從數據文件生成偏移量（offsets）的 Python 腳本。

### 5. `tests/`
單元測試代碼，確保庫的功能在不同環境下都能正確運作。

## 關鍵文件說明

- **`include/RE/Skyrim.h`**: 
  這是一個彙總頭文件，包含了大多數常用的 RE 類別定義。通常在插件中包含此文件即可。
- **`include/RE/Offsets_RTTI.h` & `Offsets_VTABLE.h`**:
  定義了大量引擎類的 RTTI（運行時類型信息）和虛函數表（VTable）的偏移量。
- **`CMakeLists.txt`**: 
  項目的構建配置文件，定義了庫的編譯方式和依賴關係。
- **`CommonLibSSE.natvis`**: 
  Visual Studio 的調試可視化配置文件，方便在調試時查看 CommonLibSSE 類對象的內容。
