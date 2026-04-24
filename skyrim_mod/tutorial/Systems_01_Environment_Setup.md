# 01. 環境準備與項目建立

在開始編寫代碼之前，我們需要配置一套現代化的 C++ 開發環境。

## 1. 必備工具

- **Visual Studio 2022**: 選擇“使用 C++ 的桌面開發”工作負載。
- **CMake (3.24+)**: 用於管理項目構建。
- **Git**: 用於獲取項目模板。
- **Vcpkg**: 這是 C++ 的包管理器，`CommonLibSSE-NG` 依賴它來管理庫。

## 2. 獲取項目模板

最快的方法是基於已有的模板開始。

1. 打開終端機（PowerShell 或 CMD）。
2. 克隆一個乾淨的模板（以 `SKSE_Template_HelloWorld` 為例）：
   ```bash
   git clone https://github.com/lucas-feliciano/skse-template-helloworld.git MyFirstPlugin
   cd MyFirstPlugin
   ```

## 3. 配置 Vcpkg 與依賴

模板目錄中應該有一個 `vcpkg.json` 文件。它定義了項目需要的依賴項。

確保您已經安裝了 `vcpkg` 並設置了環境變量 `VCPKG_ROOT`。
如果您還沒有安裝 `vcpkg`：
```bash
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
# 設置 VCPKG_ROOT 到該目錄
```

## 4. 項目初始化 (CMake)

在 `MyFirstPlugin` 目錄下，使用 CMake 生成 Visual Studio 工程：
```bash
# 使用 CMake 預設配置（如果模板支持）
cmake --preset vs2022-windows-vcpkg
```
這會自動下載 `CommonLibSSE-NG` 並配置好所有的包含路徑 (Include Paths)。

## 5. 打開項目

現在，雙擊生成的 `.sln` 文件打開 Visual Studio，或者直接在 VS 中選擇“打開文件夾”。

您的項目結構應該如下：
- `CMakeLists.txt`: 構建配置文件。
- `vcpkg.json`: 依賴清單。
- `src/`: 存放你的 C++ 源代碼。
- `include/`: 存放你的頭文件。

> **下一步**: 前往 [階段二：代碼開發實戰](02_Coding_Basics.md) 開始編寫你的第一個功能。
