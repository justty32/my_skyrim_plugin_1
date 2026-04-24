# 05. 終極挑戰：動態導航與尋路 (Dynamic Navigation)

在純代碼生成的城鎮中，最大的痛點是 NPC 會頻繁撞牆，因為底層的導航網格（Navmesh）是寫死的。本教學將探討如何利用 `NavMeshObstacleManager` 進行「導航網格剪裁（Navmesh Cutting）」，讓 NPC 學會避開你生成的建築物。

## 1. 核心邏輯：障礙物聲明
引擎提供了一種機制，允許我們告訴 AI：「這裡原本可以走，但現在被一塊大石頭（或房屋）擋住了。」
這個機制依賴於物體模型（NIF）中具備特定的碰撞層（Collision Layer），通常是 `L_NAVCUT` 或標記了 `bhkObstacle`。

## 2. 代碼實現：激活導航障礙

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

void MakeObstacleActive(RE::TESObjectREFR* a_buildingRef) {
    if (!a_buildingRef) return;

    // 1. 確保 3D 模型已經加載
    auto rootNode = a_buildingRef->Get3D();
    if (!rootNode) {
        SKSE::log::warn("建築的 3D 模型尚未加載，無法剪裁導航網格。");
        return;
    }

    // 2. 更新碰撞過濾器
    // 這會強制物理引擎重新評估該物體的碰撞體，
    // 並向 NavMeshObstacleManager 發送訊號，切斷下方的尋路網格。
    a_buildingRef->UpdateCollisionFilter();

    // 進階：有時需要手動通知導航管理器
    // (注意：這部分 API 在不同版本的 CommonLibSSE 中可能有所不同，需查閱 NavMeshObstacleManager)
    
    RE::DebugNotification("建築已註冊為導航障礙，NPC 將繞道而行。");
}
```

## 3. 實戰建議與限制
-   **模型要求**: 不是隨便一個模型生成後調用 `UpdateCollisionFilter` 就會剪裁 Navmesh。該模型必須在 Creation Kit 或 NifSkope 中設定好相應的 `NavCut` 邊界框。
-   **性能消耗**: 動態剪裁 Navmesh 是極度消耗 CPU 的操作。如果你瞬間生成了 100 棟房子並同時剪裁，遊戲會嚴重卡頓。建議採用**「異步/分批生成」**的策略。
---

## 4. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/N/NavMeshObstacleManager.h` - 導航障礙物管理器。
- `include/RE/T/TESObjectREFR.h` - 碰撞更新接口。

### 推薦使用的函數
- `RE::TESObjectREFR::UpdateCollisionFilter()`：通知引擎重新計算對象物理，並標記為導航剪裁物。
- `RE::NavMeshObstacleManager::GetSingleton()`：獲取導航管理單例。
- `RE::TESObjectREFR::Get3D()`：獲取 NiAVObject，確保物理數據已加載。
- `RE::NiAVObject::UpdateDescending(...)`：遞歸更新模型節點物理標記。