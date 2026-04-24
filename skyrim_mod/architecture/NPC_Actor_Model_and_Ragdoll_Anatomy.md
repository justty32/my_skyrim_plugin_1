# Skyrim Actor 模型與物理架構：骨骼、種族與布娃娃系統

與靜態物品模型不同，`RE::Actor` 的模型是一個高度動態的、由多個組件依附在一個「骨骼（Skeleton）」上構成的複合體。

---

## 1. 模型的核心：骨骼系統 (The Skeleton)

所有的 Actor 視覺組件都掛載在一套骨骼節點樹上。
- **類別**: `RE::NiNode` (通常指向 `skeleton.nif`)
- **原始碼**: `include/RE/N/NiNode.h`
- **結構**: 骨骼定義了所有動作的關節。例如：`NPC Root`, `NPC Spine`, `NPC Head`。
- **種族影響**: 不同種族使用不同的骨骼。
    - 巨人、人類、巨龍擁有完全不同的骨骼結構（節點數量與名稱不同）。
    - 種族（`TESRace`）定義了骨骼文件的路徑。

---

## 2. 蒙皮與分組：Biped 系統 (Body Meshes)

Actor 的肉體並非一個整體，而是分為多個「槽位（Slots）」。
- **原始碼**: `include/RE/B/BGSBipedObjectForm.h`
- **組成內容**:
    - **頭部 (Head)**: 包含 FaceGen 數據、眼球、頭髮。
    - **身體 (Torso)**: 軀幹蒙皮。
    - **手部/足部 (Hands/Feet)**: 獨立的 Mesh。
- **動態換裝**: 當你給 NPC 穿上護甲時，引擎會隱藏「身體槽位」原本的皮膚 Mesh，並在同一個骨骼位置掛載護甲的 Mesh。

---

## 3. 種族（Race）對外觀的決定性作用

`RE::TESRace` 是 Actor 視覺的總控。
- **原始碼**: `include/RE/T/TESRace.h`
- **決定因素**:
    - **預設模型**: 定義該種族男性/女性的裸體 Mesh。
    - **身高 (Height)**: 不同種族的基礎身高縮放。
    - **材質覆蓋 (Skin Data)**: 定義皮膚的紋理（如：獸人綠色皮膚、諾德人白色皮膚）。

---

## 4. 布娃娃系統 (Ragdoll / Havok Physics)

布娃娃系統負責處理 Actor 失去意識或死亡後的物理模擬。
- **類別**: `RE::bhkRagdollSystem`
- **原始碼**: `include/RE/B/bhkRagdollSystem.h`
- **工作原理**:
    1.  **物理約束 (Constraints)**: 在骨骼節點之間建立「鉸鏈（Hinges）」或「球窩接頭（Ball-and-Socket）」，限制關節旋轉的角度。
    2.  **狀態切換**: 
        - 當 Actor 活著時，模型受「動畫 (Animation)」驅動，物理引擎只計算簡單的碰撞。
        - 當 Actor 死亡時，引擎調用 `SetRagdoll(true)`，動畫停止，控制權移交給 Havok 物理引擎，讓角色根據重力和慣性倒下。
    3.  **碰撞體 (Collision Shapes)**: 每個骨骼塊（如大腿、小臂）都有一個不可見的物理簡化體（通常是膠囊體），用於與地面碰撞。

---

## 5. C++ 插件中的操控點

### A. 操作骨骼節點
你可以獲取特定骨骼並手動旋轉它，實現「實時望向目標」或「動態體型調整」。
```cpp
auto root = actor->Get3D();
auto neck = root->GetObjectByName("NPC Neck");
if (neck) {
    auto neckNode = neck->AsNode();
    // 修改 neckNode->local.rotate 實現扭頭
}
```

### B. 偵測布娃娃觸發
監聽 Actor 死亡事件，並在 Ragdoll 激活時施加一個衝擊力（Impulse），實現「擊飛」效果。

---

## 6. 核心類別原始碼標註

- **`RE::TESRace`**: `include/RE/T/TESRace.h` - 種族定義與視覺模板。
- **`RE::BGSBipedObjectForm`**: `include/RE/B/BGSBipedObjectForm.h` - 槽位與蒙皮管理。
- **`RE::NiNode`**: `include/RE/N/NiNode.h` - 骨骼節點容器。
- **`RE::bhkRagdollSystem`**: `include/RE/B/bhkRagdollSystem.h` - 布娃娃物理核心。
