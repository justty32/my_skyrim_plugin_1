# Skyrim 任務與對話交互架構：生命週期、行為觸發與獎勵

在 SKSE 插件開發中，任務（Quests）與對話（Dialogue）通常是遊戲邏輯的載體。本篇將解析如何透過 C++ 控制這些系統。

---

## 1. 任務系統架構 (Quest System)

任務在引擎中由 `RE::TESQuest` 類別表示。

- **原始碼**: `include/RE/T/TESQuest.h`
- **生命週期控制**:
    - **開始任務**: `quest->Start()`。這會初始化任務腳本並將其置於“運行”狀態。
    - **停止/結束任務**: `quest->Stop()`。這會將任務標記為已完成或失敗，並清理動態數據。
    - **階段控制 (Stages)**: `quest->SetStage(std::uint16_t a_stage)`。這是推動任務進度的核心方法。
- **目標管理 (Objectives)**:
    - **原始碼**: `include/RE/B/BGSQuestObjective.h`
    - 任務目標是 UI 上顯示的清單。透過 `SetObjectiveDisplayed(index, true)` 來更新玩家的日誌。

---

## 2. 對話與邏輯鏈接 (Dialogue Logic)

對話不僅是文字，它是觸發 C++ 邏輯的“開關”。

- **對話數據結構**:
    - **`RE::TESTopic`**: 對話的話題藍圖。(`include/RE/T/TESTopic.h`)
    - **`RE::TESTopicInfo`**: 對話的具體回應與腳本關聯。(`include/RE/T/TESTopicInfo.h`)
- **交互流程**:
    1.  玩家選擇一個對話選項。
    2.  引擎執行與該 `TopicInfo` 關聯的 **Result Script** (Papyrus)。
    3.  **C++ 介入**: 
        - **方案 A**: 在 Result Script 中調用你用 C++ 註冊的 `Native` 函數。
        - **方案 B**: 透過 `BSTEventSink<RE::MenuOpenCloseEvent>` 監聽對話選單，捕捉特定狀態。

---

## 3. 讓 NPC 做出動作 (Animation & AI)

當對話選擇發生後，讓 NPC 做出動作（如點頭、交出物品或逃跑）。

- **動畫觸發**:
    - **原始碼**: `include/RE/I/IAnimationGraphManagerHolder.h`
    - 使用 `actor->NotifyAnimationGraph("EventName")`。常見的事件名包括 `ShoutStart`, `IdleWarmHands`, `StaggerStart`。
- **AI 強制**:
    - 透過修改 `Actor` 的 `AIProcess` 或直接執行一個 `TESPackage` 來讓 NPC 走向某個位置。

---

## 4. 物品獎勵與清單操作 (Inventory)

- **獲取物品**:
    - **原始碼**: `include/RE/T/TESObjectREFR.h`
    - 使用 `player->AddObjectToContainer(item, extraData, count, sourceContainer)`。
- **動態合成**: 
    - 如果獎勵是動態生成的（如附魔武器），需要先獲取 `ExtraDataList` 並進行修改。

---

## 5. 技術總結：實戰執行鏈 (Execution Chain)

一個典型的“完成對話 -> NPC 做動作 -> 獲取獎勵 -> 結束任務”的 C++ 實現鏈條如下：

```cpp
// 假設這是你在 C++ 中註冊給對話調用的 Native 函數
void OnDialogueComplete(RE::StaticFunctionTag*, RE::Actor* a_npc, RE::TESQuest* a_quest) {
    auto player = RE::PlayerCharacter::GetSingleton();

    // 1. 推動任務到最後階段
    if (a_quest) {
        a_quest->SetStage(100); // 假設 100 是結束階段
    }

    // 2. 讓 NPC 做出“交出物品”的動作
    if (a_npc) {
        a_npc->NotifyAnimationGraph("IdleGiveItem"); 
    }

    // 3. 給予玩家物品 (PlaceHolder: 鋼鐵長劍)
    auto reward = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0001397E);
    if (player && reward) {
        player->AddObjectToContainer(reward, nullptr, 1, nullptr);
    }
    
    RE::DebugNotification("任務已完成：獲得鋼鐵長劍");
}
```

---

## 6. 核心類別原始碼標註

- **`RE::TESQuest`**: `include/RE/T/TESQuest.h` - 任務邏輯核心。
- **`RE::BGSStoryManager`**: `include/RE/B/BGSStoryManager.h` - 處理動態任務觸發（如：當玩家殺死 10 個敵人時自動開始任務）。
- **`RE::TESTopicInfo`**: `include/RE/T/TESTopicInfo.h` - 對話後的腳本觸發點。
- **`RE::InventoryChanges`**: `include/RE/I/InventoryChanges.h` - 處理複雜的物品交換與裝備邏輯。
```
