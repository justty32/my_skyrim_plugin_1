# Skyrim 架構解析：對話與任務的定義與存儲機制

在 Skyrim 中，任務（Quests）與對話（Dialogue）並非孤立的文本，而是一套互相關聯的「邏輯狀態機」。本篇將從二進制記錄（Records）的角度解析它們是如何被定義並持久化存儲的。

---

## 1. 任務系統的骨架：`TESQuest` (QUST)

一個任務在 ESP/ESM 中以 `QUST` 記錄的形式存在，它是所有對話、別名和腳本的容器。

### A. 邏輯組件
- **Stages (階段)**: 定義了任務的進度點（如：10, 20, 100）。每個階段都可以觸發一段 Papyrus 代碼片段（Fragment）。
- **Objectives (目標)**: UI 上顯示給玩家看的文字，與階段掛鉤。
- **Aliases (別名)**: 任務的「變量槽」，定義了誰是受害者、哪裡是目標地點。
- **Scenes (場景)**: 用於控制 NPC 之間的表演、對話與動作。

### B. 存儲方式
- **靜態 (ESP)**: 定義任務的 ID、優先級、初始階段和條件（Conditions）。
- **動態 (ESS)**: 存儲任務當前運行到哪個階段、哪些別名已經被填充（Filled）、哪些目標已完成。

---

## 2. 對話系統的層級：`DIAL` 與 `INFO`

Skyrim 的對話並非線性文本，而是由「話題」與「回應」構成的樹狀結構。

### A. 話題 (Topic - `DIAL` 記錄)
- **原始碼**: `include/RE/T/TESTopic.h`
- **功能**: 它是對話的分類。例如「關於黑棘家族」就是一個話題。
- **類型**: 
    - `Top-level`: 玩家主動點擊的選項。
    - `Combat / Favor`: NPC 自動觸發的喊話。

### B. 回應 (Topic Info - `INFO` 記錄)
- **原始碼**: `include/RE/T/TESTopicInfo.h`
- **功能**: NPC 具體說出的那句話。
- **內容**:
    - **語音數據**: 指向 `.fuz` 或 `.wav` 文件的路徑。
    - **字幕**: 具體的文字內容。
    - **條件 (Conditions)**: 這是關鍵！定義了這句話 NPC 能不能說（例如：玩家必須是吸血鬼，或者任務必須在第 20 階段）。

---

## 3. 任務與對話的鏈接機制

對話通常是「寄生」在任務之中的。

1.  **分組**: 在 ESP 中，大部分 `DIAL` 記錄都會歸屬於某個 `QUST`。
2.  **觸發**: 當你點擊一個對話選項時，引擎會掃描當前所有「運行中（Running）」任務下的話題。
3.  **過濾**: 引擎根據條件（Conditions）過濾掉不符合的話題，最後將剩餘的顯示在對話選單中。

---

## 4. 存檔 (ESS) 中的持久化

這是為什麼「任務卡住」難以修復的原因：

- **別名鎖定**: 一旦任務開始，別名所綁定的 NPC 會被標記為 `ChangeForm` 並寫入存檔，防止其被回收。
- **腳本堆棧**: 任務腳本的所有變量狀態都保存在存檔中。如果你修改了 ESP 裡的腳本，但舊的變量數據依然殘留在存檔裡，就會發生衝突。
- **Said Flag**: 引擎會記錄每個 `INFO` 記錄是否被 NPC 說過，以便處理「只說一次」的邏輯。

---

## 5. C++ 插件中的精確介入點

透過 CommonLibSSE-NG，你可以繞過這些複雜的數據結構：

- **獲取當前話題**: 
    ```cpp
    auto topicManager = RE::MenuTopicManager::GetSingleton();
    auto currentTopic = topicManager->lastTopic; // 獲取當前正在說的话題
    ```
- **檢查任務階段**:
    ```cpp
    if (myQuest->GetCurrentStageID() == 10) {
        // 執行插件邏輯
    }
    ```

---

## 6. 核心類別原始碼標註

- **`RE::TESQuest`**: `include/RE/T/TESQuest.h` - 任務總控。
- **`RE::TESTopic`**: `include/RE/T/TESTopic.h` - 話題定義。
- **`RE::TESTopicInfo`**: `include/RE/T/TESTopicInfo.h` - 具體對話行。
- **`RE::MenuTopicManager`**: `include/RE/M/MenuTopicManager.h` - 對話運行時管理器。
- **`RE::Condition`**: `include/RE/C/Condition.h` - 決定對話/任務是否生效的條件數據。
