# Skyrim 對話話題機制：DIAL 與 INFO 的深度運作邏輯

在 Skyrim 中，對話（Dialogue）被抽象為一系列的「話題（Topics）」。理解這套機制是修改對話內容或實現動態語音的基礎。

---

## 1. 數據結構：DIAL (TESTopic)
`DIAL` 記錄是話題的入口。
- **原始碼**: `include/RE/T/TESTopic.h`
- **屬性**:
    - **ID**: 用於邏輯引用的 FormID。
    - **Type**: 
        - `Top-level`: 玩家可以在選單中點擊的選項。
        - `Combat/Favor/Hello`: 引擎自動觸發的喊話话题。
    - **Priority**: 當多個話題衝突時，優先級決定誰先顯示。

---

## 2. 具體回應：INFO (TESTopicInfo)
一個 `DIAL` 話題下可以掛載多個 `INFO` 記錄。
- **原始碼**: `include/RE/T/TESTopicInfo.h`
- **邏輯**: `INFO` 才是 NPC 真正說出的那句話。
- **組成內容**:
    - **Response**: 字幕文字與關聯的語音路徑。
    - **Conditions (關鍵)**: 決定這個 NPC 此刻會不會選中這句 `INFO`。
        - 例如：NPC 必須在雪漫城、NPC 必須是玩家的朋友。

---

## 3. 運行時篩選流程 (Filtering Workflow)

當玩家點擊 E 鍵與 NPC 開始攀談時，引擎執行以下「過濾算法」：

1.  **收集 (Collect)**: 引擎獲取該 NPC 身上所有「運行中任務（Running Quests）」的所有話題。
2.  **分類 (Sort)**: 按 `DIAL` 的優先級排列。
3.  **條件掃描 (Validation)**: 
    - 遍歷每個話題下的 `INFO`。
    - 逐條檢查 `Conditions`。
    - 只有**至少包含一條**合法 `INFO` 的 `DIAL` 話題才會顯示在對話選單中。
4.  **展示**: 玩家看到過濾後的話題清單。

---

## 4. 對話棧與狀態 (Dialogue Stack)

- **`Said` 標誌**: 引擎會紀錄哪些 `INFO` 已經被說過。如果標記了「只說一次」，該 `INFO` 之後會被永久過濾。
- **靜態對話 vs. 動態對話**:
    - **靜態**: ESP 裡寫死的。
    - **動態**: 透過 C++ 插件手動推入對話棧。

---

## 5. C++ 插件中的操控

透過 `RE::MenuTopicManager` 單例，你可以即時掌握對話狀態：

```cpp
void MonitorDialogue() {
    auto topicManager = RE::MenuTopicManager::GetSingleton();
    
    // 獲取當前 NPC 正在說的那句 INFO
    auto currentInfo = topicManager->lastTopic; 
    
    if (currentInfo) {
        // 你可以透過 INFO 獲取它所屬的 DIAL (話題)
        auto parentTopic = currentInfo->parentTopic;
        SKSE::log::info("當前話題 ID: {:X}", parentTopic->formID);
    }
}
```

---

## 6. 核心類別原始碼標註

- **`RE::TESTopic`**: `include/RE/T/TESTopic.h` - 話題藍圖。
- **`RE::TESTopicInfo`**: `include/RE/T/TESTopicInfo.h` - 具體對白與條件。
- **`RE::MenuTopicManager`**: `include/RE/M/MenuTopicManager.h` - 運行時對話調度中心。
- **`RE::DialogueResponse`**: `include/RE/D/DialogueResponse.h` - 封裝了文字與聲音文件的結構。

---

## 7. 技術總結
話題系統是典型的「一對多」結構：**一個 `DIAL` 包含多個 `INFO`**。引擎的角色是作為一個過濾器，不斷地根據環境變量剔除掉不合格的 `INFO`，最後呈現給玩家最合適的劇情反饋。
