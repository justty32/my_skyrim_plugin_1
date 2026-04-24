# 25. 進階實戰：動態任務別名修改 (Alias Stealing)

本教學展示如何在任務已經運行時，透過 C++ 強行更換任務的「目標」或「地點」。這對於製作「任務干擾」或「多分支動態劇情」模組非常有用。

## 1. 核心邏輯
1.  找到目標 `RE::TESQuest`。
2.  獲取其內部的 `RE::BGSRefAlias` (引用別名)。
3.  調用 `ForceRefTo()` 函數強行注入一個新的實體。

## 2. 代碼實現：強行替換暗殺目標

```cpp
#include <RE/Skyrim.h>

void HijackQuestTarget(RE::FormID a_questID, RE::Actor* a_newTarget) {
    // 1. 獲取正在運行的任務 (PlaceHolder ID)
    auto targetQuest = RE::TESForm::LookupByID<RE::TESQuest>(a_questID);
    
    if (targetQuest && targetQuest->IsRunning()) {
        // 2. 獲取名為 "Target" 的別名 (索引通常需要查閱 ESP)
        // 原始碼: include/RE/B/BGSBaseAlias.h
        auto alias = targetQuest->GetAlias(0); // 假設索引 0 是目標 NPC
        auto refAlias = stl::net_cast<RE::BGSRefAlias*>(alias);

        if (refAlias) {
            // 3. 獲取舊目標 (供邏輯判斷)
            auto oldTarget = refAlias->GetReference();
            
            // 4. 強行更換目標
            // 這會立刻更新任務日誌和地圖標記到新的 NPC 身上
            refAlias->ForceRefTo(a_newTarget);
            
            RE::DebugNotification(fmt::format("任務目標已從 {} 變更為 {}", 
                oldTarget->GetName(), a_newTarget->GetName()).c_str());
        }
    }
}
```

## 3. 關鍵 API 標註
-   **`RE::BGSRefAlias`**: 任務別名的核心類別。`include/RE/B/BGSRefAlias.h`
-   **`ForceRefTo()`**: 這是動態修改任務內容的「核武器」，它會繞過所有的填充條件。
-   **`GetAlias()`**: 根據索引獲取任務組件。

## 4. 警告與建議
-   **腳本衝突**: 如果任務的原生腳本正在監聽舊目標的死亡事件，更換目標可能導致任務無法正確結算。
-   **UI 延遲**: 雖然地圖標記會更新，但有時對話選單需要重新加載才能反映出目標的變化。
-   **推薦用法**: 配合 `StoryManager` 的監聽器使用。當你偵測到某個討伐任務開始時，立刻用 C++ 將目標換成一個更具挑戰性的 Boss。
