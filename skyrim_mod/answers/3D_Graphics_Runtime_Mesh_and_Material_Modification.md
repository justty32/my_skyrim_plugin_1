# 運行時網格與材質動態修改架構 (Runtime Mesh & Material Modification)

在 Skyrim 運行時（Runtime）直接修改 3D 模型的幾何形狀（Mesh）與材質（Material），是突破靜態 NIF 限制的終極技術。這需要直接操作引擎內存中的 **場景圖 (Scene Graph)**。

---

## 1. 核心渲染架構：NetImmerse / Gamebryo

Skyrim 的 3D 物件在內存中是以樹狀結構（Node Tree）存在的：
- **`RE::NiNode`**: 容器節點，包含變換矩陣（位置、旋轉、縮放）與子節點。
- **`RE::BSTriShape` (SE版) / `RE::NiTriShape` (LE版)**: 實際包含三角形網格（頂點、索引）的幾何節點。
- **`RE::BSLightingShaderProperty`**: 附加在幾何節點上的材質與著色器屬性。

---

## 2. 運行時材質修改 (Material & Texture)

這是相對安全且常用的技術。

### A. 替換紋理集 (TextureSet)
- **原理**: 每個 `BSLightingShaderProperty` 都包含一個指向 9 張貼圖（Diffuse, Normal, Specular 等）的指針數組。
- **實作**: 透過 C++ 找到目標 `BSTriShape`，獲取其著色器屬性，並將其 Diffuse 貼圖路徑替換為新路徑。

### B. 修改著色器參數 (Shader Values)
- **應用**: 動態改變物件的發光顏色（Emissive Color）、光澤度（Glossiness）或透明度（Alpha）。
- **注意**: 修改後必須呼叫特定的刷新函數，否則渲染器（Renderer）不會將新數據提交給 GPU。

---

## 3. 運行時網格修改 (Vertex/Mesh Manipulation)

這是極度危險且高階的操作。

### A. 直接修改頂點數據 (Vertex Data)
- **原理**: 透過 `RE::BSTriShape` 可以獲取底層的頂點數組指針。你可以遍歷每一個頂點 `(X, Y, Z)`，並實時進行數學運算（如：拉伸、扭曲、頂點動畫）。
- **挑戰 1 - 緩衝區鎖定 (Buffer Locking)**: 頂點數據可能位於 VRAM (顯示記憶體) 中。在修改前可能需要進行緩衝區映射 (Map/Unmap)，如果操作不當會導致遊戲瞬間閃退 (CTD)。
- **挑戰 2 - 邊界框 (Bounding Box)**: 修改頂點後，物件的視覺邊界框必須重新計算 (`UpdateBound()`)，否則當物件中心不在玩家視角內時，即使拉伸的頂點還在畫面上，整個物件也會被錯誤剔除（Culling，突然消失）。

### B. 物理與視覺的脫節 (The Havok Desync)
- **致命問題**: 你修改的只是**視覺網格 (BSTriShape)**，而決定碰撞的是 **Havok 物理網格 (bhkRigidBody)**。
- **後果**: 如果你在運行時把一把短劍的頂點拉長成大劍，視覺上它變長了，但**攻擊距離與碰撞判定依然是短劍**。
- **解法**: 運行時修改 Havok 碰撞體極其困難，通常只能透過縮放整個 `NiNode` 來勉強同步，或者放棄物理碰撞精確度（僅用於純視覺特效）。

---

## 4. 刷新渲染管線

無論是修改材質還是網格，修改內存數據後，必須通知引擎：「這東西改變了，請重新畫」。
- **代碼**: 通常調用 `NiAVObject::Update(RE::NiUpdateData{})` 或修改標誌位強制渲染器更新。

---

## 5. 核心類別原始碼標註

- **`RE::BSTriShape`**: 幾何網格的核心類別。
- **`RE::BSLightingShaderProperty`**: 著色器與材質。
- **`RE::BSShaderTextureSet`**: 貼圖集合。

---
*文件路徑：architectures/classified/3D_Graphics/Answers/Runtime_Mesh_and_Material_Modification.md*
