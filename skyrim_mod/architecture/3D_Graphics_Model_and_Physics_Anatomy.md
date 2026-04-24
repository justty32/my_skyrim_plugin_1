# Skyrim 模型與物理架構：NIF 構造、材質與 Havok 屬性

在 Skyrim 中，一個物品在視覺上呈現出的樣子及其在物理世界中的行為，是由 NIF (NetImmerse Format) 文件與 Havok 物理引擎共同定義的。本篇將解析模型從幾何體到物理性質的解剖結構。

---

## 1. 模型的核心：幾何體 (Mesh / Shapes)

幾何體決定了物品的「形狀」。
- **類別**: `RE::NiTriShape` 或 `RE::BSTriShape` (SE/AE 版本常用)
- **原始碼**: `include/RE/N/NiTriShape.h` & `include/RE/B/BSTriShape.h`
- **組成內容**:
    - **Vertices (頂點)**: 定義空間中的點。
    - **Indices (索引)**: 定義點如何連成三角形。
    - **UV Maps**: 定義 2D 貼圖如何包裹在 3D 模型上。

---

## 2. 視覺表現：材質與渲染器 (Shader Properties)

決定物品看起來是金屬、木頭還是發光的魔法物體。
- **核心類別**: `RE::BSLightingShaderProperty`
- **原始碼**: `include/RE/B/BSLightingShaderProperty.h`
- **關鍵組件**:
    - **Textures (貼圖)**: 透過 `BSShaderTextureSet` 定義 Diffuse, Normal, Specular 等路徑。
    - **Material Type**: 定義材質的反射率、光滑度。
    - **Emissive Color (自發光)**: 讓物品在黑暗中發光（如附魔武器）。

---

## 3. 特效掛載 (Special Effects)

模型可以攜帶動態特效。
- **粒子系統 (NiParticleSystem)**: 用於火焰、煙霧或魔法光塵。
- **紋理動畫 (NiTextureTransform)**: 讓流動的水或流動的魔法紋路產生位移。
- **類別**: `RE::NiAVObject` 子類。

---

## 4. 物理性質：Havok 碰撞體 (Collision & Physics)

這決定了物品掉在地上是「咚」的一聲還是「乒」的一聲，以及它有多重。
- **類別**: `RE::bhkCollisionObject`
- **原始碼**: `include/RE/B/bhkCollisionObject.h`
- **物理組件**:
    - **Shape (碰撞形狀)**: 簡化的幾何體（如方塊、球體或複雜凸包），用於計算碰撞。
    - **Mass (質量)**: 決定了物品被撞擊時的移動慣性。
    - **Material (物理材質)**: 這是一個關鍵枚舉（如 `MAT_METAL`, `MAT_WOOD`），決定了碰撞音效、火花效果以及摩擦力。
    - **原始碼**: `include/RE/M/MaterialIDs.h`

---

## 5. C++ 插件中的動態訪問

透過 C++，你可以直接操作模型樹（Scene Graph）：

```cpp
void AnalyzeModel(RE::TESObjectREFR* a_ref) {
    auto root = a_ref->Get3D(); // 獲取 NiAVObject*
    if (!root) return;

    // 遍歷所有節點，尋找特定的 Mesh
    RE::BSVisit::TraverseScenegraphGeometries(root, [](RE::BSTriShape* a_geometry) {
        auto shaderProp = a_geometry->GetShaderProperty();
        if (shaderProp) {
            // 在這裡動態修改貼圖或光照屬性
        }
        return RE::BSVisit::BSVisitControl::kContinue;
    });
}
```

---

## 6. 技術總結：模型是由什麼組成的？

1.  **Scene Graph (NiNode)**: 樹狀結構，組織所有部分。
2.  **Geometry (BSTriShape)**: 幾何蒙皮。
3.  **Shader (BSLightingShaderProperty)**: 材質外觀。
4.  **Collision (bhkCollisionObject)**: 物理實體。
5.  **Extra (NiExtraData)**: 存儲額外信息，如接點、動畫標籤。

## 7. 核心類別原始碼標註

- **`RE::NiAVObject`**: `include/RE/N/NiAVObject.h` - 所有可見/物理對象的祖先。
- **`RE::BSTriShape`**: `include/RE/B/BSTriShape.h` - 現代幾何體核心。
- **`RE::bhkWorld`**: `include/RE/B/bhkWorld.h` - Havok 物理世界容器。
- **`RE::BSLightingShaderProperty`**: `include/RE/B/BSLightingShaderProperty.h` - 材質控制中心。
