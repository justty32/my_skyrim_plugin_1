# 15. 高級實戰：操作物品的 ExtraData (附魔與強化)

在教程 06 中，我們只是添加了基礎物品。現在我們要學習如何給予玩家一把「自定義附魔」或「打磨過」的長劍。

## 1. 核心概念：ExtraDataList
-   **原始碼**: `include/RE/E/ExtraDataList.h`
-   物品的基礎數據（如傷害）是共享的，但「這把劍加了 10 點火傷」是存在 `ExtraData` 裡的。

## 2. 代碼實現：給予一把強化過的劍

```cpp
void GiveEnhancedSword() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto swordBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0001397E);

    if (player && swordBase) {
        // 1. 創建一個新的 ExtraData 列表
        auto extraList = new RE::ExtraDataList();

        // 2. 添加打磨數據 (Health 代表物品耐久/強化等級，1.0 是標準，2.0 是傳奇)
        auto extraHealth = new RE::ExtraHealth(2.0f); 
        extraList->Add(extraHealth);

        // 3. 添加自定義名稱
        auto extraText = new RE::ExtraTextDisplayData("弒神之刃");
        extraList->Add(extraText);

        // 4. 添加到玩家背包
        player->AddObjectToContainer(swordBase, extraList, 1, nullptr);
    }
}
```

---

# 16. 高級實戰：動態導航剪裁 (Navmesh Cutting)

在教程 07 中提到，生成房屋後 NPC 會穿牆。這是因為導航網格（Navmesh）是靜態的。

## 1. 解決方案：使用 Obstacle (障礙物)
Skyrim 引擎支持「導航網格障礙物」。

-   **原理**: 為你的生成物添加一個具備 `Collision` 的 `Obstacle` 標籤。
-   **實現**: 
    1.  確保你的房屋模型在製作時（NIF 文件）設置了正確的 `bhkObstacle` 屬性。
    2.  在 C++ 中，當對象生成後，確保其 `loadedData` 中的物理體被激活。

```cpp
void EnableNavmeshCut(RE::TESObjectREFR* a_ref) {
    auto activeNode = a_ref->Get3D();
    if (activeNode) {
        // 強制物理系統更新，這會告訴 AI 這裡現在是不可通行的
        a_ref->UpdateCollisionFilter();
    }
}
```
> **注意**: 完美的動態 Navmesh 修改需要 `RE::NavMeshObstacleManager`，這涉及複雜的內存操作。
