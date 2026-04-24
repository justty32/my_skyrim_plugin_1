# 深度解析：Actor 骨骼系統 (Skeleton & Hierarchy)

骨骼是 Actor 的「靈魂支架」。在 Skyrim 中，無論一個角色看起來是什麼樣子，它的動作和物理行為都受制於這套不可見的節點樹。

---

## 1. 骨骼的載體：`RE::NiNode`
- **文件路徑**: 預設為 `Meshes\actors\character\character assets\skeleton.nif`。
- **原始碼**: `include/RE/N/NiNode.h`
- **結構**: 一個典型的類人骨骼包含約 100~200 個節點。
    - **`NPC Root [Root]`**: 整個模型的根坐標。
    - **`NPC Pelvis [Pelv]`**: 盆骨，通常是身體重心。
    - **`NPC Head [Head]`**: 頭部節點，所有的 FaceGen 模型都掛載在此。

---

## 2. 骨骼的層級繼承 (Inheritance)
骨骼節點之間存在嚴格的父子關係。
- **移動**: 如果你旋轉 `NPC Spine2`，那麼掛載在其下的手臂、肩膀和頭部都會隨之擺動。
- **座標空間**: 每個節點都有自己的 `local` 坐標（相對於父節點）和 `world` 坐標（相對於遊戲世界）。

---

## 3. 動畫與骨骼的綁定 (Skinning/Weighting)
幾何體（Mesh）是如何跟著骨骼動的？
- **權重 (Weights)**: 頂點數據中包含了權重信息。例如：手臂上的某個頂點，50% 受大臂骨骼影響，50% 受小臂骨骼影響。
- **類別**: `RE::NiSkinInstance` 管理著 Mesh 與骨骼節點的關聯。

---

## 4. C++ 操控技巧
你可以透過 C++ 尋找並強行旋轉骨骼：
```cpp
auto rootNode = actor->Get3D();
auto headNode = rootNode->GetObjectByName("NPC Head")->AsNode();
if (headNode) {
    // 強迫 NPC 歪頭 (修改旋轉矩陣)
    // headNode->local.rotate = ...
}
```

---

## 5. 核心類別原始碼標註
- **`RE::NiNode`**: `include/RE/N/NiNode.h` - 節點容器。
- **`RE::NiAVObject`**: `include/RE/N/NiAVObject.h` - 具備坐標與名字的基礎對象。
- **`RE::NiTransform`**: `include/RE/N/NiTransform.h` - 坐標轉換矩陣。
