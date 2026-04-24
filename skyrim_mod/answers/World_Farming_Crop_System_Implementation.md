# Skyrim 農作物與種植系統實作架構 (Farming & Crop System)

在 Skyrim 中（特別是 Hearthfire DLC 之後），種植與農作物系統主要圍繞著 **Flora（植物/採集物）** 與 **Planter（種植盆）** 展開。要添加自製農作物或建立一套種植系統，需要結合物件替換與時間推進機制。

---

## 1. 核心數據表單與類別

### A. 採集物與原料
- **`RE::TESFlora`**: 遊戲中可採集的植物實體（如：山花、小麥）。它包含了「未採集」與「已採集」兩種 NIF 模型狀態。
- **`RE::IngredientItem`**: 採集後玩家獲得的物品（煉金材料或食物）。在 Flora 的數據中會指定採集時給予哪種 Ingredient。

### B. 泥土與種植盆
- **Hearthfire 種植機制**: 遊戲利用 `BYOHPlanter` 腳本。當玩家點擊肥沃的泥土（Fertile Soil）時，會打開一個種子清單，選擇種子後，系統會隱藏泥土，並在該座標生成對應的 Flora。

---

## 2. 實作自製農作物

1.  **建立 Ingredient (作物果實)**: 設定食用效果、重量與價值。
2.  **建立 Flora (生長的植物)**:
    - 指定模型：未採集時的模型（滿是果實），以及採集後的模型（光禿禿的莖葉）。
    - 連結產出物：設定 Harvest 產出物為你剛建立的 Ingredient。
    - 收成音效：設定為拔草或採摘的聲音。
3.  **加入種植清單 (FormList)**: 將你的 Ingredient 加入 `BYOHPlantableItems` 清單，這樣玩家就能在花盆中種下它。

---

## 3. 農作物生長時間循環 (Growth Cycle)

農作物需要時間成長。這通常依賴於遊戲內的全局時間變數 `GameDaysPassed`。

- **Flora 重置**: Skyrim 引擎會自動重置 Cell 內的 Flora（通常是遊戲時間 10 到 30 天，取決於 iDaysToRespawnVendor 等設定）。
- **自定義生長腳本 (C++ / Papyrus)**:
    如果想要更精細的階段（種子 -> 幼苗 -> 成熟）：
    1. 準備三個不同階段的 Static/Flora 模型。
    2. 當種下種子時，記錄當前的 `GameDaysPassed`。
    3. 透過 `RegisterForSingleUpdateGameTime(24.0)` 每隔一天檢查一次。
    4. 達到特定天數後，使用 `PlaceAtMe` 生成下一階段的模型，並 `Disable` 舊模型。

---

## 4. C++ 中的採集攔截 (SKSE)

如果你想製作「自動收割機」或修改採集邏輯，可以 Hook 引擎的互動事件。

```cpp
// 攔截玩家採集動作
void HarvestCrop(RE::TESFlora* a_flora, RE::Actor* a_harvester) {
    // 檢查植物是否已被採集
    if (!a_flora->GetHarvested()) {
        // 執行自定義邏輯：例如根據玩家農業技能給予雙倍產出
        
        // 標記為已採集，並更新 3D 外觀
        a_flora->SetHarvested(true);
        a_flora->Update3DModel();
    }
}
```

---

## 5. 核心類別原始碼標註

- **`RE::TESFlora`**: `include/RE/T/TESFlora.h` - 植物與採集點。
- **`RE::IngredientItem`**: `include/RE/I/IngredientItem.h` - 煉金/食物原料。
- **`RE::Calendar`**: `include/RE/C/Calendar.h` - 獲取遊戲內時間。

---
*文件路徑：architectures/classified/World/Farming_Crop_System_Implementation.md*
