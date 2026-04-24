# Skyrim 自定義房屋與家具製作系統 (Custom Housing & Crafting)

要實作一套類似《異塵餘生 4》或進階版《爐火》的房屋建設系統，需要結合 **Static Object 放置**、**Recipe (配方) 系統**與 **持久化座標儲存**。

---

## 1. 核心組件

### A. 建設模式 (Workshop Mode)
- **原理**: 透過 SKSE 攔截特定按鍵，將玩家切換至一個特殊的「編輯狀態」。
- **實現**: 
    1. 隱藏玩家武器。
    2. 生成一個「虛擬光標」（通常是一個隱形的 XMarker，附帶一個高亮效果的 Shader）。
    3. 玩家移動光標，系統在座標點生成家具的預覽模型（Ghost Model）。

### B. 家具配方 (`RE::BGSConstructibleObject`)
- **本質**: 在遊戲中這被稱為 COBJ。它定義了製作一件家具需要多少木材、鐵錠等原材料。
- **自定義**: 你可以建立一個專屬的「建築工作台」，只顯示類別為家具的 COBJ。

---

## 2. 家具放置邏輯 (Placement Logic)

當玩家按下「確認放置」時，C++ 插件需執行以下步驟：

1.  **計算座標**: 取得虛擬光標的 `Position` 與 `Rotation`。
2.  **生成實體**: 
    ```cpp
    auto factory = RE::IFormFactory::GetConcreteFormFactory<RE::TESObjectSTAT>();
    auto furniture = factory->Create(); // 或者從 FormList 中查找現有模型
    
    // 在世界中生成
    auto handle = player->PlaceAtMe(furniture, 1, false, false);
    ```
3.  **導航網格切換 (Navmesh Cutting)**: 這是最難的部分。大的家具會阻擋 NPC 路徑。你需要使用 `RE::NavMeshObstacleManager` 動態添加障礙物塊，讓 NPC 繞道。

---

## 3. 持久化存儲 (Persistence)

由於玩家放置的家具不在原始 ESP 中，必須手動保存。
- **方案**: 監聽 `kSaveGame` 事件。
- **內容**: 將所有玩家放置的家具 `FormID`、`Position`、`Rotation` 序列化並寫入 `SKSE Co-save` 檔案中。
- **讀取**: 在 `kLoadGame` 時，解析檔案並重新生成所有物件。

---

## 4. 核心類別原始碼標註

- **`RE::BGSConstructibleObject`**: `include/RE/B/BGSConstructibleObject.h` - 配方。
- **`RE::TESObjectSTAT`**: 靜態物件（家具模型）。
- **`RE::NavMeshObstacleManager`**: 處理物理障礙與 AI 路徑的同步。

---
*文件路徑：architectures/classified/World/Custom_Housing_and_Furniture_System.md*
