# 實戰教學：自定義 UI 與 Flash (Scaleform) 整合 (Custom UI)

本教學將教你如何製作一個自定義的 HUD 介面或彈出選單，並透過 C++ 與 Flash 進行通訊。

## 難度等級與準備工作
- **難度**: 極高 (Very Hard)
- **準備工具**:
    - **Adobe Animate (Flash)**: 製作 UI 介面（需使用 ActionScript 2.0）。
    - **CommonLibSSE-NG**: 編寫 UI 控制插件。

---

## 實作步驟

### 步驟一：製作 Flash UI (.swf)
1. 在 Flash 中建立一個元件（如文字框），命名為 `myText`。
2. 在第一幀撰寫 AS2 代碼：
   ```actionscript
   function SetValue(newVal) {
       myText.text = newVal;
   }
   ```
3. 導出為 `MyUI.swf` 並放入 `Data/Interface/`。

### 步驟二：C++ 繼承 `RE::IMenu`
1. 建立一個類別 `MyMenu` 繼承自 `RE::IMenu`。
2. 在構造函數中調用 `LoadMovie` 加載 `MyUI.swf`。
3. 設置選單標誌（如：是否攔截滑鼠輸入 `kAllowCursor`）。

### 步驟三：註冊與顯示選單
1. 在插件啟動時向 `RE::UI` 單例註冊你的選單類別。
2. 使用 `RE::UIMessageQueue` 發送顯示訊息。

### 步驟四：數據通訊 (C++ -> Flash)
1. 獲取 `RE::GFxMovieView` 指針。
2. 使用 `Invoke()` 函數呼叫 Flash 內部定義的 `SetValue` 函數。

---

## 代碼實踐 (C++ - CommonLibSSE-NG)

```cpp
#include <RE/Skyrim.h>

class MyCustomMenu : public RE::IMenu {
public:
    static constexpr const char* MENU_NAME = "MyCustomMenu";

    static void Register() {
        RE::UI::GetSingleton()->Register(MENU_NAME, []() -> RE::IMenu* {
            return new MyCustomMenu();
        });
    }

    MyCustomMenu() {
        auto loader = RE::BSScaleformManager::GetSingleton();
        loader->LoadMovie(this, view, "MyUI"); // 指向 Data/Interface/MyUI.swf
    }

    void UpdateText(const char* a_str) {
        if (view) {
            RE::GFxValue val;
            val.SetString(a_str);
            view->Invoke("SetValue", nullptr, &val, 1);
        }
    }
};
```

---

## 常見問題與驗證
- **驗證方式**: 在遊戲中按下特定熱鍵，觀察是否出現自定義的 UI 視窗，且文字是否正確顯示。
- **問題 A**: UI 出現了但滑鼠點不到？
    - *解決*: 確保 `menuFlags` 中勾選了 `kAllowCursor`。
- **問題 B**: UI 背景是白色的？
    - *解決*: 在 Flash 設置中將背景設為透明，並在匯出時確認透明度設置。
- **提示**: 避免在 `OnUpdate` 中每幀更新 UI，應僅在數值改變時調用 `Invoke`。
