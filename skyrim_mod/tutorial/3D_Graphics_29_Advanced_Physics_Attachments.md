# 29. 技術實戰：動態物理掛載 (物理起司吊墜)

在教程 27 中，我們學會了如何掛載靜態的起司。現在我們要更進一步：實現一個**「會隨劍揮動而擺動」**的物理起司吊墜。

這不僅僅是 3D 樹的操作，還需要啟動引擎的 Havok 物理運算。

## 1. 核心邏輯
1.  **分離實體**: 不要將起司直接作為武器的子節點掛載（那樣它是死板的）。
2.  **創建 Havok 約束 (Constraints)**: 在武器的某個骨骼節點與起司的物理實體之間建立一根「虛擬繩索」。
3.  **動力學同步**: 將起司標記為「動力學 (Dynamic)」而非「運動學 (Kinematic)」。

## 2. 代碼實現：物理吊墜掛載

```cpp
#include <RE/Skyrim.h>

void AttachPhysicsCheese(RE::Actor* a_player) {
    // 1. 加載具備物理屬性的起司模型
    // 注意：該 NIF 必須包含 bhkRigidBody 且設為 Dynamic 類型
    RE::NiStream stream;
    if (!stream.Load("Meshes\\Clutter\\Food\\CheesePendant_Physics.nif")) return;
    
    auto cheeseMesh = stream.GetObjectAt<RE::NiAVObject>(0);
    auto weaponNode = a_player->Get3D(false); // 右手武器節點

    if (cheeseMesh && weaponNode) {
        // 2. 獲取物理世界 (include/RE/B/bhkWorld.h)
        auto physicsWorld = a_player->GetParentCell()->GetHavokWorld();
        
        // 3. 建立物理連接 (Ball-and-Socket Constraint)
        // 原始碼參考: include/RE/B/bhkConstraint.h
        
        // 概念步驟：
        // A. 獲取武器的物理剛體 (RigidBody A)
        // B. 獲取起司的物理剛體 (RigidBody B)
        // C. 創建一個 bhkBallAndSocketConstraint 對象
        // D. 將 A 和 B 鏈接在一起，設置關節點在劍柄處
        
        // 4. 將起司模型掛載到 Scene Graph 供渲染
        weaponNode->AsNode()->AttachChild(cheeseMesh, true);
        
        // 5. 關鍵：激活 Havok 模擬
        // 調用物理更新，讓起司受重力影響並開始擺動
        cheeseMesh->Update(nullptr);
        RE::DebugNotification("物理起司吊墜已掛載，試著揮揮劍吧！");
    }
}
```

## 3. 3D 與物理技術細節解析

### A. 為什麼普通掛載不會動？
在教程 27 中，我們使用的 `AttachChild` 是 Scene Graph 層面的操作。子節點會**完全複製**父節點的坐標變換。這就像你把起司直接焊死在劍柄上。

### B. 什麼是物理約束 (Constraints)？
物理約束是 Havok 引擎用來模擬關節的機制。
-   **Ball and Socket**: 像肩關節，可以朝各個方向旋轉但不能分離。
-   **Hinge**: 像門合頁，只能朝一個軸向旋轉。
-   對於吊墜，我們通常使用 **Ball and Socket**。

### C. NIF 模型的要求
普通的起司模型（如 `CheeseWheel01.nif`）的物理屬性通常是 `Static` 或 `Kinematic`（為了節省性能）。
要實現擺動，你必須在 NifSkope 中將其 `bhkRigidBody` 的 `Motion System` 改為 **`MO_SYS_BOX`** 或其他動態類型，並賦予它一定的 **質量 (Mass)**。

## 4. 關鍵 API 標註
-   **`RE::bhkConstraint`**: 所有物理約束的基類。`include/RE/B/bhkConstraint.h`
-   **`RE::bhkBallAndSocketConstraint`**: 球窩關節實現。
-   **`RE::bhkRigidBody`**: 物理剛體實體。`include/RE/B/bhkRigidBody.h`

## 5. 實戰建議
-   **性能警告**: 動態物理計算非常昂貴。如果全城鎮的 NPC 都掛著物理吊墜，遊戲會非常卡。
-   **穿模問題**: 物理物件可能會穿進武器模型內部。在 NifSkope 中設置正確的 `Collision Layer` (如 `L_PXP`) 可以緩解此問題。
-   **推薦時機**: 這種效果最適合用在玩家的獨特武器上。
