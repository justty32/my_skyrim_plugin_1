# 實戰教學：劇情任務系統設計指南 (Quest & Storyline)

本教學將引導你建立一個具備動態別名 (Alias) 與自動觸發功能的 Skyrim 任務。

## 難度等級與準備工作
- **難度**: 中階 (Medium)
- **準備工具**:
    - **Creation Kit (CK)**: 核心任務編輯工具。
    - **Papyrus**: 編寫任務階段腳本。

---

## 實作步驟

### 步驟一：建立基礎 Quest 框架
1. 在 CK 的 `Quest` 窗口建立新項目。
2. 設置 `ID` 與 `Name`。
3. 勾選 `Start Game Enabled` (若希望遊戲一開始就啟動) 或透過腳本啟動。

### 步驟二：配置 Quest Stages (階段)
1. 在 `Stages` 標籤頁建立序列（如 10, 20, 30）。
2. 為每個階段撰寫 `Log Entry`，這是玩家在任務日誌中看到的內容。
3. 在 `Result Script` 區塊編寫進入該階段時要執行的代碼（例如：`SetObjectiveDisplayed(10)`）。

### 步驟三：使用 Quest Aliases (別名)
1. 這是讓任務動態化的關鍵。建立一個 Alias，命名為 `TheTarget`。
2. 設置尋找條件（例如：`Find Nearest NPC` 且屬於 `BanditFaction`）。
3. 任務啟動後，引擎會自動將地圖上符合條件的 NPC 綁定到此 Alias。

### 步驟四：整合 Story Manager (自動觸發)
1. 在 `Story Manager Storage` 中找到對應的事件（如 `Kill Actor`）。
2. 將你的 Quest 加入該事件的分支中。
3. 設置條件（例如：玩家殺死的對象必須是「龍」），滿足後任務會自動啟動。

---

## 代碼實踐 (Papyrus 任務腳本)

```papyrus
Scriptname MyQuestScript extends Quest

QuestProperty Property Itself Auto
ReferenceAlias Property TargetAlias Auto

Function StartTheQuest()
    Itself.Start()
    Itself.SetStage(10)
EndFunction

; 當目標被殺死時（在 Alias 腳本中）
Event OnDeath(Actor akKiller)
    Debug.Notification("目標已死亡，任務推進！")
    Itself.SetStage(100) ; 完成任務
EndEvent
```

---

## 常見問題與驗證
- **驗證方式**: 在控制台輸入 `sqv [QuestID]` 檢查任務狀態與 Alias 是否已正確綁定到 NPC。
- **問題 A**: 任務日誌不顯示？
    - *解決*: 檢查是否已調用 `SetObjectiveDisplayed()` 且對應的 Objective Index 正確。
- **問題 B**: Alias 找不到對象？
    - *解決*: 檢查 `Find Nearest` 的半徑設定，或該 NPC 是否位於已加載的 `uGridsToLoad` 範圍內。
- **安全建議**: 關鍵角色務必勾選 `Essential` 或 `Protected`，防止任務因意外死亡而中斷。
