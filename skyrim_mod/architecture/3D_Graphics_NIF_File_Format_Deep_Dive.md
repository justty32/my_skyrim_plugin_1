# Skyrim 3D 模型深度解析：.nif 檔案格式與 Block 結構

NIF (NetImmerse Format) 是 Skyrim 引擎所使用的核心 3D 模型格式。它不是像 OBJ 或 FBX 那樣的單一幾何體文件，而是一個「**可執行數據包**」，包含了一套完整的場景圖（Scene Graph）。

---

## 1. NIF 的塊狀構造 (Block-based Structure)

NIF 文件由一系列相互鏈接的「塊（Blocks）」組成。你可以將其想像成一個二進制鏈表。

- **Header**: 儲存版本號（Skyrim SE 通常為 20.2.0.7）。
- **Blocks**: 每個塊都有一個唯一的索引（Index）。
- **Footer**: 包含塊的類型名稱映射表。

---

## 2. 核心 Block 類型解析

### A. NiNode (節點塊)
- **作用**: 構建模型的架構。它不包含形狀，只包含坐標、旋轉和縮放信息。
- **原始碼**: `include/RE/N/NiNode.h`
- **鏈接**: `NiNode` 儲存了一個子塊索引列表（Children），形成了樹狀結構。

### B. BSTriShape (幾何塊)
- **作用**: 儲存真正的「肉體」。
- **原始碼**: `include/RE/B/BSTriShape.h`
- **內容**: 包含頂點緩衝（Vertices）和索引緩衝（Triangles）。

### C. BSLightingShaderProperty (渲染塊)
- **作用**: 告訴引擎如何塗抹貼圖。
- **原始碼**: `include/RE/B/BSLightingShaderProperty.h`
- **鏈接**: 它通常作為 `BSTriShape` 的一個屬性掛載。

### D. bhkCollisionObject (物理塊)
- **作用**: 包含 Havok 物理碰撞數據。
- **內容**: 簡化版的幾何體（Shape），定義了物體在遊戲中被踢到或踩到時的物理邊界。

---

## 3. NIF 的解析流程 (Engine Perspective)

1.  **讀取 Header**: 檢查版本是否兼容。
2.  **實例化 0 號 Block**: 通常是根節點（Root NiNode）。
3.  **遞歸遍歷**: 引擎根據索引，逐一加載子節點、幾何體和材質屬性。
4.  **建立 Scene Graph**: 在內存中生成 `RE::NiAVObject` 樹。

---

## 4. C++ 插件開發中的 NIF 修改

作為開發者，你通常不需要手動解析二進制 NIF，而是使用 `RE::NiStream`：

```cpp
RE::NiStream nifStream;
if (nifStream.Load("Meshes\\Example.nif")) {
    // 獲取根節點
    auto root = nifStream.GetObjectAt<RE::NiNode>(0);
    
    // 你可以手動遍歷並修改 Block
    for (auto& child : root->children) {
        if (child && child->name == "WeaponEffect") {
            // 找到特定的 Block 並進行操作
        }
    }
}
```

---

## 5. 為什麼 NIF 是開發中的「大坑」？

- **嚴格的版本限制**: 如果你嘗試在 SE 中加載舊版（如 Morrowind）的 NIF，遊戲會立刻崩潰。
- **材質路徑**: NIF 內部寫死了材質路徑。如果貼圖文件不存在，模型會變成紫色的「缺失貼圖」外觀。
- **骨骼綁定**: 如果 NIF 包含蒙皮（Skinning），它必須與遊戲中的骨骼節點（NiNode）名稱完全精確匹配。

## 6. 推薦工具
- **NifSkope**: 查看和編輯 NIF 結構的最強工具。
- **Outfit Studio**: 用於調整蒙皮與槽位。

## 7. 核心類別原始碼標註
- **`RE::NiStream`**: `include/RE/N/NiStream.h` - 核心讀取流。
- **`RE::NiObject`**: `include/RE/N/NiObject.h` - 所有 Block 的基類。
- **`RE::BSGeometry`**: `include/RE/B/BSGeometry.h` - 幾何體數據基類。
