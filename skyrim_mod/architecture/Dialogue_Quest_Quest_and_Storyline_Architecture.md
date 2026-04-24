# Skyrim 劇情與任務系統架構 (Quest & Storyline Architecture)

Skyrim 的劇情核心是 `RE::TESQuest`。它不只是「任務清單」，更是管理腳本、對話與動態數據的「控制器」。

---

## 1. 核心類別：`RE::TESQuest`

- **原始碼**: `include/RE/T/TESQuest.h`
- **功能**: 
    - 儲存變數（透過 Papyrus 腳本）。
    - 驅動對話。
    - 監聽世界事件。

---

## 2. 劇情組件 (Components)

### A. Quest Stages (劇情階段)
- **Index**: 通常以 10, 20, 30 排序。
- **Log Entry**: 顯示在玩家任務日誌中的文字。
- **Script Fragment**: 當進入該階段時執行的代碼（如：生成敵人、播放音樂）。

### B. Quest Aliases (任務別名)
這是動態劇情的精華。
- **作用**: 任務不需要寫死「去找白漫城的守衛」，而是定義一個 Alias 名為 `TargetGuard`，並設置條件讓引擎在任務啟動時自動尋找「距離玩家最近且存活的守衛」。
- **動態綁定**: Alias 可以附加專用的腳本，當該角色死亡時觸發特定邏輯。

### C. Objectives (任務目標)
- 指引玩家前進的 UI 標記（如：擊敗強盜 0/5）。

---

## 3. Story Manager (劇情管理器)

如何讓任務「自動觸發」？
- **Event-Driven**: 當玩家殺死一條龍、進入一座城市或竊取一件物品時，Story Manager 會檢查所有符合條件的 Quest 並啟動它們。
- **優點**: 避免在全局腳本中頻繁檢測，節省性能。

---

## 4. C++ 插件與任務互動

```cpp
void AdvanceQuest(const char* a_editorID) {
    auto dataHandler = RE::TESDataHandler::GetSingleton();
    auto quest = dataHandler->LookupForm<RE::TESQuest>(a_editorID, "YourMod.esp");
    
    if (quest && quest->IsEnabled()) {
        // 將任務推進到下一個階段
        quest->SetCurrentStageID(20);
    }
}
```

---

## 5. 開發建議

1.  **結構化**: 保持 Quest Stage 邏輯簡單，將複雜運算移至 C++ 或專用的 Papyrus 函數。
2.  **安全性**: 善用 Alias 的 `Essential` 標籤，防止關鍵劇情 NPC 在任務完成前意外死亡。
3.  **清理**: 當劇情結束時，務必調用 `Stop()` 釋放被 Alias 佔用的內存空間。

---

## 6. 核心類別原始碼標註

- **`RE::TESQuest`**: `include/RE/T/TESQuest.h`
- **`RE::BGSBaseAlias`**: `include/RE/B/BGSBaseAlias.h`
- **`RE::BGSStoryManagerTreeForm`**: 劇情觸發樹。

---
*文件路徑：architectures/classified/Dialogue_Quest/Quest_and_Storyline_Architecture.md*
