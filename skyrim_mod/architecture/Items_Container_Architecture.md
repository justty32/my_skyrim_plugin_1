# Skyrim 容器系統架構：存儲、物品欄與動態內容管理

在 Skyrim 中，容器（Containers）是數據交換的核心。無論是路邊的木桶、玩家的保險箱，還是 NPC 的背包，在引擎底層都共享著同一套管理邏輯。

---

## 1. 靜態定義：`TESObjectCONT` (CONT)

容器在 ESP/ESM 中是以 `CONT` 記錄定義的。
- **原始碼**: `include/RE/T/TESObjectCONT.h`
- **初始物品 (`TESContainer`)**: 
    - 每個容器模板都包含一個 `TESContainer` 組件，裡面列出了「初始生成的物品」及其數量。
    - **Leveled Items**: 支持放置分級清單（Leveled List），實現隨機掉落。
- **標誌位**: 定義是否可以重生物品（Respawns）、是否會自動打開蓋子（Play Animation）。

---

## 2. 運行時實例：`InventoryChanges` (核心)

當玩家打開一個容器時，引擎會從靜態模板實例化內容。負責管理這些動態數據的核心對象是 `InventoryChanges`。

- **原始碼**: `include/RE/I/InventoryChanges.h`
- **數據掛載**: 實體引用 (`TESObjectREFR`) 透過 `ExtraContainerChanges` 這個額外數據組件持有 `InventoryChanges` 的指針。
- **內容組成**:
    - 它維護著一個 `InventoryEntryData` 的清單。
    - 每個條目包含：一個 `BaseObject*`、物品數量、以及一個可選的 `ExtraDataList*`（用於存儲附魔、毒藥等個體差異）。

---

## 3. 物品的移動流程 (Transfer Logic)

當物品從一個容器移動到另一個容器（如玩家撿起蘋果）：
1.  **分離 (Detach)**: 引擎從源容器的 `InventoryEntryData` 中扣除數量。
2.  **實例化 (Instantiate)**: 如果物品具備特殊屬性（ExtraData），引擎會複製這份數據。
3.  **附加 (Attach)**: 將物品與其數據加入目標容器的 `InventoryChanges` 列表。
4.  **發送事件**: 觸發 `RE::TESContainerChangedEvent` 事件。

---

## 4. 特殊容器：Actor (角色)

在引擎底層，**`RE::Actor` 繼承了容器的操作接口**。
- **共同點**: NPC 擁有背包、可以使用 `AddObjectToContainer` 函數、受 `ExtraContainerChanges` 管理。
- **不同點**: 
    - Actor 具備「裝備位置（Equip Slots）」。其 `InventoryEntryData` 包含一個 Flag，標記該物品是否正穿戴在身上。
    - 死亡後的 Actor 遺體會轉化為一個靜態容器狀態，直到被引擎清理（見教學 14：安全轉化）。

---

## 5. 商業交互：商箱 (Merchant Chests)

為什麼你不能直接在商人身上偷到他賣的所有東西？
- **機制**: 商人的交易物品通常存儲在另一個**隱形的物理容器**中（商箱）。
- **鏈接**: 商人的 `Actor` 實體透過派系（Faction）與商箱的所有權掛鉤。當你點擊交易時，選單顯示的是商箱的內容，而非商人的隨身背包。

---

## 6. C++ 插件開發中的操作

### A. 獲取容器內的所有物品
```cpp
void ListContainerItems(RE::TESObjectREFR* a_container) {
    auto inventory = a_container->GetInventory();
    for (auto& [item, data] : inventory) {
        auto& [count, entry] = data;
        SKSE::log::info("物品: {}, 數量: {}", item->GetName(), count);
    }
}
```

### B. 強制清空容器
```cpp
void ClearContainer(RE::TESObjectREFR* a_container) {
    auto changes = a_container->GetInventoryChanges();
    if (changes) {
        // 調用引擎內部的清理函數
        // ...
    }
}
```

---

## 7. 核心類別原始碼標註

- **`RE::TESObjectCONT`**: `include/RE/T/TESObjectCONT.h` - 靜態容器定義。
- **`RE::InventoryChanges`**: `include/RE/I/InventoryChanges.h` - 運行時物品欄管理器。
- **`RE::InventoryEntryData`**: `include/RE/I/InventoryEntryData.h` - 單個物品條目數據。
- **`RE::ExtraContainerChanges`**: `include/RE/E/ExtraContainerChanges.h` - 數據掛載組件。
- **`RE::TESContainer`**: `include/RE/T/TESContainer.h` - ESP 中的初始清單。
