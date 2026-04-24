# 深度解析：動態任務生成與別名填充 (Alias Filling)

動態任務（Radiant Quests）的核心在於「隨機性」與「關聯性」。這一切都是透過 **Quest Aliases** 系統實現的。

## 1. 什麼是別名 (Aliases)？
- **原始碼**: `include/RE/B/BGSBaseAlias.h`
- **機制**: 別名是任務中的「變量」。
    - 任務定義：殺死 **[Alias:目標]** 在 **[Alias:地點]**。
    - 運行時：引擎將 **[目標]** 替換為“斷牙洞穴的強盜頭領”。

## 2. 別名填充邏輯 (The Filling Process)
當任務啟動時，引擎執行以下步驟來填充別名：
1.  **查找條件**: 根據別名定義的條件（例如：必須是女性 NPC，且目前活著）。
2.  **空間過濾**: 通常會選擇玩家當前所在區域附近的目標。
3.  **依賴繼承**:
    - 別名 A (地點): 選取一個隨機洞穴。
    - 別名 B (目標): 選取屬於 **別名 A** 內部的一個 NPC。
4.  **鎖定 (Locking)**: 一旦 NPC 被填入別名，他會獲得 `Persistent` 標籤，防止被引擎回收，直到任務結束。

## 3. 數據傳遞：Scripting & Event Data
- **Event Data**: 故事管理器觸發任務時，會傳遞一個「數據包」。
    - 如果是 `Kill Actor` 觸發，數據包包含「誰殺了誰」。
    - 任務可以直接將「被害者」這個數據填入別名。

## 4. C++ 介入：精確的任務操控
- **動態別名修改**: 透過 C++ 獲取一個運行中任務的別名指針，強行將目標從 NPC_A 換成 NPC_B。
- **偵測填充失敗**: 如果一個任務因為找不到符合條件的 NPC 而啟動失敗，你可以透過 Hook 獲取錯誤代碼並手動生成一個 NPC（教學 18）來滿足填充需求。

---

## 5. 核心類別原始碼標註
- **`RE::BGSQuestAlias`**: `include/RE/B/BGSQuestAlias.h` - 任務變量基類。
- **`RE::BGSRefAlias`**: `include/RE/B/BGSRefAlias.h` - 對應世界引用的別名。
- **`RE::BGSLocAlias`**: `include/RE/B/BGSLocAlias.h` - 對應區域位置的別名。
