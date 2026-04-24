# 03. 終極挑戰：模塊化拼接系統 (Snapping System)

既然你的模塊已經內建了 Navmesh，那麼開發的核心就變成了：**如何讓 C++ 像玩樂高積木一樣，精確地將模塊 A 的出口與模塊 B 的入口對齊。**

## 1. 核心概念：接點 (Sockets / Snap Points)

要在 C++ 中拼接模組，你需要在每個模組物件上定義「接點」。
-   **接點數據**: 包含相對於模組中心點的 `座標 (Offset)` 與 `旋轉 (Direction)`。
-   **對齊算法**: 當你放置模塊 B 時，其座標應為：`模塊 A 座標 + 接點偏移量`。

## 2. 代碼實現：對齊拼接邏輯

```cpp
#include <RE/Skyrim.h>

struct SnapPoint {
    RE::NiPoint3 pos;
    float rotation; // 弧度
};

// 假設我們有兩個模塊的 Base對象
// 模塊 A: 轉角城牆, 模塊 B: 直線城牆
void SnapModuleBToA(RE::TESObjectREFR* a_moduleA, RE::TESBoundObject* a_baseB) {
    // 1. 定義 A 模塊末端的接點 (這通常預先測量好或存儲在配置中)
    // 假設接點在 A 的 X 軸正方向 500 單位處
    SnapPoint socketA = { {500.0f, 0.0f, 0.0f}, 0.0f };

    // 2. 獲取模塊 A 在世界中的位置與旋轉
    RE::NiPoint3 posA = a_moduleA->GetPosition();
    float rotA = a_moduleA->GetAngle().z;

    // 3. 計算模塊 B 的目標位置 (涉及旋轉矩陣運算)
    // 這裡簡化為二維平面運算
    RE::NiPoint3 targetPosB;
    targetPosB.x = posA.x + (socketA.pos.x * cos(rotA) - socketA.pos.y * sin(rotA));
    targetPosB.y = posA.y + (socketA.pos.x * sin(rotA) + socketA.pos.y * cos(rotA));
    targetPosB.z = posA.z + socketA.pos.z;

    // 4. 生成並放置模塊 B
    auto moduleB = a_moduleA->PlaceAtMe(a_baseB, 1, true, false);
    if (moduleB) {
        moduleB->SetPosition(targetPosB);
        moduleB->SetAngle(0.0f, 0.0f, rotA + socketA.rotation);
        
        // 5. 關鍵：激活物理與導航連通
        moduleB->UpdateCollisionFilter();
    }
}
```

## 3. 解決 Navmesh 連通難點

即使每個模組都有 Navmesh，如果它們是獨立生成的 `REFR`，NPC 在跨越拼接縫隙時可能會猶豫。

-   **解決方案 A (Edge Linking)**: 確保你的模組在設計時，Navmesh 的邊緣剛好切在模型邊界上。Skyrim 引擎會嘗試將極其靠近的兩個 Navmesh 邊緣視為「連通」。
-   **解決方案 B (Navmesh Obstacle Manager)**: 
    -   **原始碼**: `include/RE/N/NavMeshObstacleManager.h`
    -   調用該管理器的 `Update()`，讓引擎重新掃描拼接區域的物理碰撞，強制刷新尋路路徑。

## 4. 總結
模塊化拼接的成功取決於**數學精度**。---

## 5. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/N/NiPoint3.h` - 向量坐標運算。
- `include/RE/N/NiMatrix3.h` - 旋轉矩陣處理。
- `include/RE/N/NavMeshObstacleManager.h` - 導航更新管理。

### 推薦使用的函數
- `RE::TESObjectREFR::GetPosition()` / `GetAngle()`：獲取基點模塊狀態。
- `RE::TESObjectREFR::SetPosition()` / `SetAngle()`：精確對齊新模塊。
- `RE::NavMeshObstacleManager::GetSingleton()->Update()`：強制重新掃描導航物理。
- `RE::TESObjectREFR::UpdateCollisionFilter()`：刷新物理層與導航剪裁層。
