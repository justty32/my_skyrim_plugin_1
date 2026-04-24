# 實戰教學：農作物與種植系統實作 (Farming & Crop System)

本教學將引導你建立一套完整的農場種植系統，包含自製農作物、生長週期管理以及自動採集功能。

## 難度等級與準備工作
- **難度**: 中階 (Medium)
- **準備工具**:
    - **Creation Kit (CK)**: 建立 `Flora` 與 `Ingredient` 數據。
    - **Papyrus**: 編寫基礎生長邏輯。
    - **CommonLibSSE-NG**: (進階) 攔截採集事件。

---

## 實作步驟

### 步驟一：建立農作物基礎 Form
1. 在 CK 中建立一個 `Ingredient`（如：金黃小麥），設定其效果與重量。
2. 建立一個 `Flora`（植物），指定生長狀態的模型（成熟期）與採集後的模型（枯萎期）。
3. 將 `Flora` 的 `Produce` 連結到剛建立的 `Ingredient`。

### 步驟二：加入種植盆系統
1. 將你的作物果實 (Ingredient) 加入 `BYOHPlantableItems` 這個 FormList。
2. 這會讓玩家在 Hearthfire 的種植盆選單中看到並種下該作物。

### 步驟三：實作生長階段邏輯
如果你想要更真實的生長過程（種子 -> 幼苗 -> 成熟）：
1. 建立三個不同階段的物件（可以都是 `Static` 或 `Flora`）。
2. 使用腳本記錄種下的遊戲時間 (`GameDaysPassed`)。
3. 每天檢查一次，若超過設定天數，則 `Disable` 當前模型並 `Enable` 下一階段的模型。

### 步驟四：優化採集體驗
1. (進階) 在 C++ 中 Hook 採集動作，根據玩家的「農耕等級」給予額外的產量。

---

## 代碼實踐 (C++ - CommonLibSSE-NG)

攔截採集並增加產量的範例：

```cpp
#include <RE/Skyrim.h>

void OnHarvest(RE::TESFlora* a_flora, RE::Actor* a_harvester) {
    if (a_harvester->IsPlayer()) {
        // 假設玩家具備雙倍收成天賦
        auto produce = a_flora->produceItem;
        if (produce) {
            a_harvester->AddObjectToContainer(produce, nullptr, 1, nullptr); // 額外多給一個
            RE::DebugNotification("豐富的收成！你額外獲得了物品。");
        }
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 在花盆種下作物，使用控制台命令 `set timescale to 10000` 加速時間，觀察作物是否正常生長成熟。
- **問題 A**: 植物採集後不重置？
    - *解決*: Skyrim 的植物重置取決於 `iDaysToRespawnVendor` (預設 10-30 天)。你可以透過腳本手動調用 `SetHarvested(false)`。
- **問題 B**: 模型沒變化？
    - *解決*: 檢查 Flora 的 `3D Data`，確保指定了 `Harvested` 與 `Un-harvested` 兩種狀態的模型路徑。
