# Skyrim 自定義 UI 與 Flash (Scaleform) 整合指南

Skyrim 的所有介面（HUD、選單、清單）都是基於 **Adobe Flash (ActionScript 2.0)** 並透過 **Scaleform** 引擎渲染的。要添加自定義 UI，你需要結合 Flash 資源製作與 C++ 插件邏輯。

---

## 1. 核心類別架構

- **`RE::IMenu`**: 所有自定義選單的基底類別。你需要繼承它來定義選單的行為。
- **`RE::GFXMovieView`**: 代表 Flash 檔案（.swf）的視圖對象，負責與 ActionScript 通訊。
- **`RE::UI`**: 管理所有選單的單例，負責選單的註冊、開啟與關閉。

---

## 2. 實作流程

### A. 製作 Flash 資源 (.swf)
1.  **環境**: 使用 Adobe Animate (或早期 Flash Professional)，語言必須設為 **ActionScript 2.0**。
2.  **腳本**:
    - **從 C++ 接收數據**: 定義一個全局函數，如 `function SetLabelText(text) { myLabel.text = text; }`。
    - **傳送數據回 C++**: 使用 `fscommand` 或特定對象：`skse.SendEvent("MyButtonClick", "Param1");` (需要 SKSE 支持)。

### B. C++ 繼承 `RE::IMenu`
你需要建立一個類別來代表你的選單：

```cpp
class MyCustomMenu : public RE::IMenu {
public:
    static constexpr const char* MENU_NAME = "MyCustomMenu";

    MyCustomMenu() {
        auto ui = RE::UI::GetSingleton();
        auto loader = RE::BSScaleformManager::GetSingleton();
        
        // 加載位於 Data/Interface/MyMenu.swf 的檔案
        loader->LoadMovie(this, view, "MyMenu");
        
        // 設置選單屬性 (是否攔截滑鼠、是否暫停遊戲)
        menuFlags.set(RE::UI_MENU_FLAGS::kPauseGame, false);
        menuFlags.set(RE::UI_MENU_FLAGS::kAllowCursor, true);
    }

    // 處理來自 ActionScript 的調用
    void Accept(RE::FxDelegateHandler::CallbackProcessor* a_processor) override {
        // 註冊函數讓 Flash 調用
    }
};
```

### C. 註冊與開啟選單
在插件初始化時註冊選單：
```cpp
RE::UI::GetSingleton()->Register("MyCustomMenu", []() -> RE::IMenu* {
    return new MyCustomMenu();
});
```

開啟選單：
```cpp
RE::UIMessageQueue::GetSingleton()->AddMessage("MyCustomMenu", RE::UI_MESSAGE_TYPE::kShow, nullptr);
```

---

## 3. C++ 與 UI 的雙向通訊

### 1. 從 C++ 呼叫 Flash (Push)
```cpp
void UpdateUIText(const std::string& a_text) {
    if (view) {
        RE::GFxValue arg;
        arg.SetString(a_text.c_str());
        view->Invoke("SetLabelText", nullptr, &arg, 1);
    }
}
```

### 2. 從 Flash 呼叫 C++ (Pull)
透過 `RE::FxDelegateHandler` 註冊回呼函數，這讓 Flash 中的按鈕點擊能直接觸發 C++ 中的邏輯。

---

## 4. 技術挑戰與細節

- **深度與圖層 (Depth)**: 選單的 `depth` 決定了它是在物品欄上方還是下方。
- **輸入上下文 (Input Context)**: 開啟 UI 時，通常需要切換到 `Cursor` 模式，否則玩家移動滑鼠會同時旋轉鏡頭。
- **資源路徑**: 預設搜尋路徑為 `Data/Interface/`。
- **性能**: 避免在每一幀都使用 `Invoke` 更新大量數據，應改用事件驅動或僅在數據變更時更新。

---

## 5. 核心類別原始碼標註

- **`RE::IMenu`**: `include/RE/I/IMenu.h`
- **`RE::UI`**: `include/RE/U/UI.h`
- **`RE::GFxMovieView`**: `include/RE/G/GFxMovieView.h`
- **`RE::BSScaleformManager`**: `include/RE/B/BSScaleformManager.h`

---
*文件路徑：architectures/classified/Systems/Custom_UI_and_Flash_Integration.md*
