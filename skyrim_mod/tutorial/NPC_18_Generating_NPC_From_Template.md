# 18. 技術實戰：透過 NPC 模板生成實體

在製作召喚類、同伴類或動態世界模組時，你經常需要從一個現有的數據藍圖（`TESNPC`）生成一個實體（`Actor`）。

## 1. 核心概念：`TESNPC` vs `Actor`
-   **`RE::TESNPC`**: 這是靜態數據（模板），定義了對象的臉、預設屬性、種族和技能。
-   **`RE::Actor`**: 這是世界中的實體，是基於模板生成的具體對象。

## 2. 代碼實現：召喚一個自定義衛兵

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

void SpawnNPCAtPlayer() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    // 1. 獲取 NPC 模板 (PlaceHolder ID: 0x00012E46 - 雪漫城衛兵)
    auto npcBase = RE::TESForm::LookupByID<RE::TESNPC>(0x00012E46);
    
    if (npcBase) {
        // 2. 在玩家位置生成實體
        // 參數：模板, 數量, 是否持久化, 是否禁用
        auto spawnedRef = player->PlaceAtMe(npcBase, 1, false, false);
        
        if (spawnedRef) {
            // 3. 轉換為 Actor 以執行進階邏輯
            auto newActor = spawnedRef->As<RE::Actor>();
            if (newActor) {
                // 讓他在一分鐘後自動消失
                // (參考高級教學 13：回收機制)
                
                // 讓他立刻變成玩家的盟友
                newActor->SetPlayerTeammate(true, false);
                
                RE::DebugNotification(fmt::format("已召喚: {}", newActor->GetName()).c_str());
            }
        }
    }
}
```

## 3. 關鍵 API 標註
-   **`RE::TESNPC`**: NPC 的基礎數據類別。`include/RE/T/TESNPC.h`
-   **`PlaceAtMe()`**: 最核心的生成函數。`include/RE/T/TESObjectREFR.h`
-   **`SetPlayerTeammate()`**: 將 NPC 設為隊友狀態。`include/RE/A/Actor.h`

## 4. 實戰建議
-   **位置偏移**: 建議使用 `newActor->SetPosition()` 稍微調整位置，防止 NPC 與玩家重疊。
-   **等級同步**: 動態生成的 NPC 默認會根據模板的等級規則自動縮放（Scaling）。
-   **回收**: 對於動態生成的 NPC，務必使用 `SetDelete(true)` 進行清理，避免存檔中累積過多“死掉的衛兵”。
