# 實戰教學：地圖載入、過場畫面與世界轉場優化 (Loading & Transitions)

本教學將探討如何優化 Skyrim 的載入體驗，包括自定義載入畫面以及透過 C++ 監聽載入狀態來執行初始化邏輯。

## 難度等級與準備工作
- **難度**: 中階 (Medium)
- **準備工具**:
    - **Creation Kit (CK)**: 用於建立 `TESLoadScreen` 數據。
    - **CommonLibSSE-NG**: 監聽引擎加載事件。

---

## 實作步驟

### 步驟一：建立自定義載入畫面
1. 在 CK 的物件窗口找到 `Miscellaneous > Load Screen`。
2. 建立新項目，選擇你想顯示的 3D 模型（如一隻新的怪物）以及背景圖片。
3. 在 `Loading Message` 欄位填入你模組的背景知識或提示。
4. 設置 `Conditions`，例如：`GetInCurrentLocation == MyCustomWorldID`，讓此畫面只在特定地點出現。

### 步驟二：監聽 Cell 加載事件
當玩家進入新區域時，通常需要執行一些代碼（如生成動態 NPC）：
1. 在 C++ 插件中繼承 `RE::BSTEventSink<RE::TESCellFullyLoadedEvent>`。
2. 在 `ProcessEvent` 中過濾目標 Cell ID。
3. 執行你的初始化邏輯。

### 步驟三：優化載入效能 (5x5 Grid Loading)
1. **uGridsToLoad**: 理解此參數對載入的影響。預設為 5，代表載入周邊 5x5 的單元。
2. **優化建議**: 避免在載入完成的一瞬間執行極高耗能的運算，應使用異步 (Asynchronous) 處理或延遲幾秒執行，以防止玩家在看到畫面時發生卡頓。

---

## 代碼實踐 (C++ - CommonLibSSE-NG)

監聽加載完成並印出資訊：

```cpp
#include <RE/Skyrim.h>

class LoadEventHandler : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
public:
    static LoadEventHandler* GetSingleton() {
        static LoadEventHandler singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override {
        if (a_event && a_event->cell) {
            auto cellName = a_event->cell->GetFullName();
            RE::ConsoleLog::GetSingleton()->Print("Cell '%s' 已完全載入！", cellName);
            
            // 此處可執行針對該區域的動態初始化
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

// 在 SKSEPluginLoad 中註冊
// RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(LoadEventHandler::GetSingleton());
```

---

## 常見問題與驗證
- **驗證方式**: 進入遊戲，快速旅行到不同城市，檢查控制台是否正確顯示 Cell 名稱。
- **問題 A**: 載入畫面一直不出現？
    - *解決*: 檢查 `Conditions`。如果沒有設置任何條件，引擎會從所有可用的載入畫面中隨機抽取。
- **問題 B**: 加載完成後遊戲閃退？
    - *解決*: 檢查 `TESCellFullyLoadedEvent` 中的邏輯是否涉及修改正在渲染的 3D 節點，建議在下一幀執行。
- **技術細節**: `uGridsToLoad` 若改為 7 或 9 會大幅增加載入時間，模組製作者應測試在預設 5 的環境下系統是否穩定。
