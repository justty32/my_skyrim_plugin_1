# 23. 進階實戰：擴充與強制沙盒行為

本教學展示如何透過 C++ 接管 NPC 的「日常生活」邏輯，讓他們在特定時刻執行你指定的沙盒動作。

## 1. 核心目標
-   強迫一個 NPC 走向並使用你動態生成的家具（如：教程 04 生成的床）。
-   手動刷新 NPC 的行為包（Package）。

## 2. 代碼實現：強迫 NPC 去休息

```cpp
#include <RE/Skyrim.h>

void ForceNPCtoSleep(RE::Actor* a_npc, RE::TESObjectREFR* a_bed) {
    if (!a_npc || !a_bed) return;

    // 1. 獲取一個現有的睡眠行為包 (PlaceHolder: 預設睡眠 Package ID)
    auto sleepPackage = RE::TESForm::LookupByID<RE::TESPackage>(0x00012345);
    
    if (sleepPackage) {
        // 2. 獲取 NPC 的 AI 進程
        auto process = a_npc->currentProcess;
        if (process) {
            // 強行將家具設置為該 Package 的執行目標
            // 注意：這通常涉及修改 AI 堆棧，最穩定的做法是臨時更改 NPC 的優先級
            
            // 3. 讓 NPC 重新評估行為
            // 參數：是否立即刷新, 是否強迫停止當前動作
            a_npc->EvaluatePackage(true, true);
            
            // 4. 動態指令：如果 NPC 就在床邊，可以直接強迫進入家具交互
            a_npc->UseFurniture(a_bed);
        }
    }
}
```

## 3. 擴充：讓 NPC 使用自定義標記 (Idle Markers)
如果你生成了一個「靠牆站」的標記，NPC 是不會主動去的。你需要在 C++ 中：
1.  攔截 NPC 的 `PickNextBehavior`（見教學 22）。
2.  在掃描範圍內優先推薦你的自定義標記點給 NPC。

---

# 24. 進階實戰：手動觸發故事管理器節點

故事管理器（Story Manager）通常是被動觸發的（如玩家升級）。本教學教你如何主動「欺騙」引擎，讓它以為發生了某個事件，從而啟動關聯的 Radiant 任務。

## 1. 核心邏輯：QueueEvent
-   使用 `RE::BGSStoryManager::QueueEvent` 函數。
-   構造對應的事件數據（Event Data）。

## 2. 代碼實現：強行觸發「僱傭兵復仇」劇情

```cpp
void TriggerBountyHunt(RE::Actor* a_targetNPC) {
    auto storyManager = RE::BGSStoryManager::GetSingleton();
    if (!storyManager) return;

    // 1. 獲取“殺死 NPC”節點的 ID (PlaceHolder)
    // 原始碼: include/RE/B/BGSStoryManager.h
    
    // 2. 準備數據包 (這部分非常依賴引擎底層結構)
    // 模擬 a_targetNPC 被殺死的事件，這會觸發所有監聽該節點的 Radiant 任務
    // storyManager->QueueEvent(RE::StoryManagerBranchNode::kKillActor, a_targetNPC, ...);
    
    RE::DebugNotification("已向命運女神發送指令：復仇任務啟動。");
}
```
> **注意**：手動 QueueEvent 涉及大量的內存指針操作，建議先從修改現有任務的 `Conditions` 入手。
