# 03. 構建與部署

代碼寫好了，現在我們要將它轉化為遊戲可以識別的 `.dll` 文件。

## 1. 編譯 (Build)

在 Visual Studio 中：
1. 將構建配置設置為 **Release** (性能更好) 或 **Debug** (方便調試)。
2. 將平台設置為 **x64** (Skyrim 是 64 位遊戲)。
3. 點擊 **生成 (Build)** -> **生成解決方案**。

如果一切順利，你應該能在 `out/` 或 `build/` 子目錄下找到生成的 `.dll` 文件（例如 `MyFirstPlugin.dll`）。

## 2. 部署到遊戲目錄

SKSE 插件必須存放在特定的目錄結構中。

1. 找到你的 Skyrim 遊戲目錄（例如 `SteamLibrary/steamapps/common/Skyrim Special Edition/Data`）。
2. 在 `Data` 文件夾下，確保有以下路徑：
   `SKSE/Plugins/`
3. 將你的 `MyFirstPlugin.dll` 複製到該文件夾中。

**最終路徑應如下：**
`.../Skyrim Special Edition/Data/SKSE/Plugins/MyFirstPlugin.dll`

## 3. 運行遊戲

1. 使用 **skse64_loader.exe** 啟動遊戲。
2. 進入遊戲存檔。
3. 按下你代碼中定義的鍵（例如 **'F'** 鍵）。
4. 檢查屏幕左上角是否出現了坐標通知！

## 4. 如何查看日誌 (Log)

如果插件沒起作用，第一件事就是檢查日誌。`CommonLibSSE-NG` 會在你的 `文檔` 文件夾下生成日誌：
`Documents/My Games/Skyrim Special Edition/SKSE/MyFirstPlugin.log`

檢查日誌中是否有錯誤信息或你自定義的輸出。

## 5. 常見問題排查

- **DLL 沒加載**: 檢查日誌路徑。確保你的插件名稱沒寫錯。
- **遊戲崩潰**: 
  - 檢查是否訪問了空指針（例如 `player` 是否為空？）。
  - 檢查是否使用了錯誤的 `FormID`。
- **版本不匹配**: 確保你在 `CMakeLists.txt` 中配置的目標版本與你的遊戲版本一致（NG 版本通常會自動處理這一點）。

## 恭喜！
你已經完成了從環境設置到開發、部署的全過程。現在你可以嘗試修改代碼，探索 `docs/` 文件夾中的其他 API，開始製作更強大的 Mod 了！
