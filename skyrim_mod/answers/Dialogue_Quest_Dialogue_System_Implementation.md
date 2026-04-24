# Skyrim 對話系統實作指南 (Dialogue System Implementation)

Skyrim 的對話系統由 `Topic`、`Info` 與 `Conditions` 三者緊密結合而成。

---

## 1. 數據層級 (Hierarchy)

1.  **Branch (對話分支)**: 對話的入口，如「關於那個傳說...」。
2.  **Topic (RE::TESTopic)**: 一個具體的主題。
3.  **Topic Info (RE::TESTopicInfo)**: NPC 實際說出的每一句台詞。一個 Topic 可以包含多個 Info，引擎會根據條件選擇最合適的一句。

---

## 2. 核心機制：Conditions (條件判定)

這是對話系統的大腦。你可以設置：
- `GetIsID`: 只有特定 NPC 能說這句話。
- `GetQuestStage`: 只有任務進行到某階段時才出現對話。
- `GetItemCount`: 玩家身上有特定物品時觸發。

---

## 3. 語音與口型同步 (Lip Sync)

- **Voice Type**: 每個 NPC 都有一個 `VoiceType`。台詞錄音檔必須存放在對應的目錄下。
- **LIP 檔案**: 紀錄了音頻頻率與口型動作的對應。沒有 LIP 檔案，NPC 說話時嘴巴不會動。
- **路徑規則**: `Data\Sound\Voice\YourMod.esp\VoiceType\TopicID_InfoID.fuz` (FUZ 是 LIP 與 XWM 音頻的壓縮格式)。

---

## 4. C++ 插件中的對話處理

通常我們不需要在 C++ 中手動「寫」對話，而是監聽對話事件來觸發遊戲邏輯：

```cpp
// 監聽對話結束事件
struct DialogueMenuHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override {
        if (a_event->menuName == RE::DialogueMenu::MENU_NAME && !a_event->opening) {
            // 對話框關閉時執行邏輯 (例如：結算報酬)
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

---

## 5. 實作流程

1.  **建立 Quest**: 對話必須依附於一個 Quest。
2.  **建立 Topic**: 選擇對話類型（如：`Player Dialogue` 或 `NPC to NPC`）。
3.  **撰寫 Response**: 輸入台詞文字。
4.  **設置鏈接**: 決定說完這句話後是結束對話，還是跳轉到另一個 Topic。
5.  **生成語音**: 使用 CK 的錄音功能或外部工具生成 `.fuz` 檔案。

---

## 6. 核心類別原始碼標註

- **`RE::TESTopic`**: `include/RE/T/TESTopic.h`
- **`RE::TESTopicInfo`**: `include/RE/T/TESTopicInfo.h`
- **`RE::DialogueMenu`**: `include/RE/D/DialogueMenu.h`

---
*文件路徑：architectures/classified/Dialogue_Quest/Dialogue_System_Implementation.md*
