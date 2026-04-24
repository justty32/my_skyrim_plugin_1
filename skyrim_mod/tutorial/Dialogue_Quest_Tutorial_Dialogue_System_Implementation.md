# 實戰教學：對話系統與語音口型同步 (Dialogue System)

本教學將指導你如何建立具備條件分支的對話，並生成能讓 NPC 嘴巴動起來的語音檔案。

## 難度等級與準備工作
- **難度**: 中階 (Medium)
- **準備工具**:
    - **Creation Kit (CK)**: 編輯對話內容。
    - **LazyVoiceFinder / Yakuzen**: 用於生成語音與口型 (LIP) 檔案。

---

## 實作步驟

### 步驟一：建立對話 Quest
1. 建立一個新 Quest，`Type` 設置為 `None`。
2. 在 `Dialogue Views` 或 `Player Dialogue` 標籤頁建立一個 `Topic`。
3. 輸入 `Prompt`（玩家看到的對話選項文字）。

### 步驟二：撰寫台詞 (Topic Info)
1. 在 Topic 下方建立 `Info`。
2. 在 `Response Text` 輸入 NPC 說出的台詞。
3. 設置 `Conditions`。例如：`GetIsID == MyCustomNPC`，確保只有你的角色會說這句話。

### 步驟三：生成 LIP 檔案 (口型同步)
1. 在 CK 中雙擊台詞，點擊 `Record` (需麥克風) 或導入 `.wav` 檔案。
2. 點擊 `Generate Lip File`。這會產生一個同名的 `.lip` 檔案。
3. 確保台詞檔案路徑正確（參考 `Data/Sound/Voice/...`）。

### 步驟四：對話導向與結算
1. 設置 `Linked To`，決定這句話說完後是結束對話，還是跳轉到另一個問題。
2. 在台詞的 `End Script` 中執行代碼（例如：給予玩家物品 `Game.GetPlayer().AddItem(Gold, 100)`）。

---

## 代碼實踐 (C++ 監聽對話結束)

如果你想在對話結束後執行複雜的 C++ 邏輯：

```cpp
#include <RE/Skyrim.h>

class DialogueWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
        // DialogueMenu 是對話框
        if (a_event->menuName == RE::DialogueMenu::MENU_NAME && !a_event->opening) {
            RE::ConsoleLog::GetSingleton()->Print("玩家剛剛結束了一段對話。");
            // 執行後續邏輯，如刷新 NPC 商店庫存
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

---

## 常見問題與驗證
- **驗證方式**: 進入遊戲，與 NPC 交談，確認對話選項是否出現，且 NPC 說話時是否有嘴型變化。
- **問題 A**: 對話選項不顯示？
    - *解決*: 檢查 `Conditions`。如果條件不滿足，對話永遠不會出現。另外，確認該 Quest 是否已啟動。
- **問題 B**: NPC 說話沒聲音或沒嘴動？
    - *解決*: 檢查檔案路徑。必須嚴格遵循 `Voice/ModName.esp/VoiceType/TopicID_InfoID.fuz` 的路徑格式。
- **提示**: 使用 `FUZ` 格式可以將語音與口型壓縮在一起，節省硬碟空間。
