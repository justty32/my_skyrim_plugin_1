# 10. 進階實戰：修改與干預 NPC 對話系統

本教學將教您如何透過 C++ 插件與 Skyrim 的對話系統交互。我們將實現：**當玩家與 NPC 對話時，攔截特定事件，並強迫 NPC 說出一段自定義文字，同時在對話選單中添加動態選項。**

## 1. 核心邏輯
1.  監聽對話選單開啟事件 (`RE::DialogueMenu`)。
2.  獲取當前正在對話的 NPC。
3.  **動態語音/字幕**: 使用 `RE::Debug::Notification` 或操作 `SubtitleManager`。
4.  **對話攔截**: 使用 Hook 技術攔截 NPC 選擇對話分支的邏輯。

## 2. 代碼實現

### 第一部分：監聽對話開啟
透過監聽 `MenuOpenCloseEvent`，我們可以知道玩家何時開始與 NPC 攀談。

```cpp
class DialogueHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
        // DialogueMenu 是對話選單的名稱
        if (a_event->menuName == RE::DialogueMenu::MENU_NAME && a_event->opening) {
            auto menu = RE::UI::GetSingleton()->GetMenu<RE::DialogueMenu>();
            if (menu) {
                // 獲取當前對話的 NPC 引用
                auto speaker = RE::MenuTopicManager::GetSingleton()->speaker.get();
                if (speaker) {
                    RE::ConsoleLog::GetSingleton()->Print("正在與 %s 對話", speaker->GetName());
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

### 第二部分：強制 NPC 顯示自定義字幕
有時候你不需要在 ESP 中建立對話，只需讓 NPC 頭頂出現一段文字。

```cpp
void ForceSubtitle(RE::Actor* a_speaker, const char* a_text) {
    // 獲取字幕管理器 (include/RE/S/SubtitleManager.h)
    auto subtitleManager = RE::SubtitleManager::GetSingleton();
    if (subtitleManager) {
        // 強制添加一條字幕
        // 參數：說話者, 文字, 顯示時長, 是否強制
        subtitleManager->AddSubtitle(a_speaker, a_text, 5.0f, true);
    }
}
```

### 第三部分：攔截並修改對話選項 (進階)
這需要 Hook `RE::DialogueMenu` 的 `ProcessMessage` 或 `Update` 方法。這是一個高度簡化的思路：

```cpp
// 偽代碼：攔截對話清單的構建
struct BuildDialogueList {
    static void thunk(RE::DialogueMenu* a_menu) {
        // 先執行原有的清單構建邏輯
        func(a_menu);

        // 獲取對話項目清單
        // 你可以動態添加一個新的 TESTopic 到清單中
        auto topicManager = RE::MenuTopicManager::GetSingleton();
        
        // 如果條件滿足，手動插入一個“搶劫”選項 (PlaceHolder ID)
        auto myCustomTopic = RE::TESForm::LookupByID<RE::TESTopic>(0xMY_TOPIC_ID);
        if (myCustomTopic) {
            // 將自定義話題加入當前的對話棧
        }
    }
    static inline REL::Relocation<decltype(thunk)> func;
};
```

## 3. 關鍵 API 標註
-   **`RE::DialogueMenu`**: 對話選單類別。`include/RE/D/DialogueMenu.h`
-   **`RE::MenuTopicManager`**: 管理當前對話話題與說話者的核心單例。`include/RE/M/MenuTopicManager.h`
-   **`RE::SubtitleManager`**: 負責處理屏幕下方字幕的顯示。`include/RE/S/SubtitleManager.h`
-   **`RE::TESTopic` / `RE::TESTopicInfo`**: 對話話題及其詳細響應的數據結構。`include/RE/T/TESTopic.h`

## 4. 實戰建議
-   **純插件限制**: 完全“新建”一段有語音、有口型的對話而不使用 ESP 是極其困難的，因為口型數據 (.lip) 和音頻路徑是高度依賴數據加載器的。
-   **替代方案**: 許多 Mod 使用 C++ 插件監聽對話，然後根據選擇的選項調用 `Actor::StartConversation()` 或 `RE::Debug::SendAnimationEvent()` 來模擬反應。
-   **動態文本**: 如果你正在製作 AI 對話 Mod，你會頻繁使用 `SubtitleManager` 來顯示 AI 生成的文本，並配合 `NotifyAnimationGraph("TalkingStart")` 來讓 NPC 動嘴。
