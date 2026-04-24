# 33. 技術實戰：憐憫之心 (條件觸發 NPC 反應)

本教學展示如何透過 C++ 監聽玩家狀態，並尋找周圍符合特定條件（性別）的 NPC 做出社交反應。

## 1. 核心邏輯
1.  **狀態監聽**: 每一秒檢查玩家的當前血量比例。
2.  **範圍掃描**: 如果血量低於 30%，掃描周圍 500 單位內的 NPC。
3.  **條件過濾**: 檢查 NPC 是否為女性、是否友善、是否能看見玩家。
4.  **行為執行**: 
    - 強迫 NPC 轉頭看向玩家（LookIK）。
    - 強迫 NPC 說出關心的話語。

## 2. 代碼實現：關心邏輯

```cpp
#include <RE/Skyrim.h>

void CheckAndTriggerCare() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    // 1. 檢查玩家血量 (百分比)
    float healthPct = player->GetActorValue(RE::ActorValue::kHealth) / player->GetBaseActorValue(RE::ActorValue::kHealth);
    
    if (healthPct < 0.3f) { // 低於 30% 血量
        
        // 2. 遍歷當前單元內的所有 NPC
        auto cell = player->GetParentCell();
        if (cell) {
            for (auto& ref : cell->runtimeData.references) {
                auto npc = ref->As<RE::Actor>();
                
                // 3. 過濾條件
                if (npc && !npc->IsPlayerRef() && !npc->IsDead()) {
                    
                    // 獲取 NPC 模板以檢查性別
                    auto npcBase = npc->GetActorBase();
                    bool isFemale = npcBase && npcBase->GetSex() == RE::SEX::kFemale;
                    
                    // 檢查距離與視線
                    float distance = player->GetPosition().GetDistance(npc->GetPosition());
                    bool canSee = npc->HasLineOfSight(player);

                    if (isFemale && distance < 500.0f && canSee) {
                        // 4. 執行反應
                        TriggerResponse(npc, "天哪，你受傷不輕！需要幫忙嗎？");
                        break; // 只讓一個 NPC 說話，防止吵雜
                    }
                }
            }
        }
    }
}

void TriggerResponse(RE::Actor* a_npc, const char* a_text) {
    // 讓 NPC 看向玩家 (LookIK)
    a_npc->SetLookAt(RE::PlayerCharacter::GetSingleton());

    // 使用 SubtitleManager 顯示文字 (參考教學 10)
    RE::SubtitleManager::GetSingleton()->AddSubtitle(a_npc, a_text, 3.0f, true);
    
    // 讓 NPC 動嘴巴
    a_npc->NotifyAnimationGraph("TalkingStart");
    
    // (進階) 如果有準備好的 TESTopicInfo，調用 a_npc->Say(topicInfo);
}
```

## 3. 技術細節解析

### A. 性別檢查
性別數據儲存在 `TESNPC`（模板）中而非 `Actor`（實體）中。這就是為什麼我們需要 `npc->GetActorBase()->GetSex()`。

### B. SetLookAt (看向目標)
這是引擎內建的 LookIK 系統。當你調用此函數後，NPC 的頸部骨骼會動態旋轉以追蹤目標，直到你調用 `ClearLookAt()`。

### C. 執行頻率
不要在 `Main::Update` 中每一幀都執行這個掃描。建議使用一個簡單的計數器，每 60 幀（約 1 秒）執行一次。

## 4. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/T/TESNPC.h` - 性別數據。
- `include/RE/S/SubtitleManager.h` - 字幕控制。
- `include/RE/A/Actor.h` - `HasLineOfSight` 與 `SetLookAt`。

### 推薦使用的函數
- `RE::Actor::GetActorBase()->GetSex()`：精確獲取 NPC 性別。
- `RE::Actor::HasLineOfSight(Target)`：檢查中間是否有障礙物遮擋。
- `RE::Actor::SetLookAt(Target)`：啟動頭部追蹤物理效果。
- `RE::SubtitleManager::AddSubtitle(...)`：在不使用 ESP 的情況下顯示對白。
