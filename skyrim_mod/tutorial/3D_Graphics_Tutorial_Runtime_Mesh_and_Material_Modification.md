# 教學：運行時動態修改網格頂點與材質 (Tutorial)

本教學將示範如何撰寫 SKSE C++ 插件，在遊戲運行時直接尋找並修改裝備或物件的 3D 數據。

---

## 難度等級：極限 (Expert+)

### 準備工具：
1. **Visual Studio 2022** + **CommonLibSSE-NG**。
2. 對 C++ 指針與內存結構有深刻理解。

---

## 步驟一：獲取目標 3D 節點
首先，我們需要從遊戲世界的實體（`TESObjectREFR` 或 `Actor`）取得它的 `NiNode`。

```cpp
RE::NiNode* GetObject3D(RE::TESObjectREFR* a_ref) {
    if (!a_ref) return nullptr;
    // 獲取 3D 根節點。如果是 NPC，可能需要指定是獲取武器還是身體
    return a_ref->Get3D(); 
}
```

---

## 步驟二：動態修改材質 (替換貼圖)
尋找目標幾何體，並替換其第一張貼圖（漫反射/Diffuse）。

```cpp
void SwapTexture(RE::NiNode* a_root, const char* a_shapeName, const char* a_newTexturePath) {
    if (!a_root) return;

    // 1. 根據名稱找到幾何節點
    auto avObj = a_root->GetObjectByName(a_shapeName);
    if (!avObj) return;

    // 2. 轉換為幾何體 (SE 版為 BSTriShape)
    auto geometry = avObj->AsGeometry();
    if (geometry) {
        // 3. 獲取 Shader 屬性
        auto effect = geometry->GetProperties().effect;
        if (effect) {
            auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect.get());
            if (lightingShader && lightingShader->textureSet) {
                // 4. 替換索引 0 (Diffuse) 的貼圖
                lightingShader->textureSet->SetTexturePath(0, a_newTexturePath);
            }
        }
    }
}
```

---

## 步驟三：動態修改網格頂點 (Mesh Deformation)
這是非常危險的操作。我們嘗試將某個幾何體的所有頂點沿 Z 軸拉長。

```cpp
void DeformMesh(RE::NiNode* a_root, const char* a_shapeName, float a_zScale) {
    if (!a_root) return;

    auto avObj = a_root->GetObjectByName(a_shapeName);
    if (!avObj) return;

    auto triShape = netimmerse_cast<RE::BSTriShape*>(avObj);
    if (triShape) {
        // 1. 嘗試獲取頂點數據 (注意：這在某些情況下可能是 null 或受保護的)
        // 在 CommonLibSSE-NG 中，存取底層 vertex data 需要使用特定的偏移或逆向結構
        // 這裡提供概念性代碼 (具體 API 可能因 CommonLib 版本而異)
        
        auto vertexCount = triShape->vertexCount;
        // 假設我們有辦法取得頂點數組指針 vertices (RE::NiPoint3*)
        /*
        for (uint32_t i = 0; i < vertexCount; ++i) {
            vertices[i].z *= a_zScale; // 拉長 Z 軸
        }
        */

        // 2. 更新邊界框 (極為重要，否則物件會憑空消失)
        // triShape->UpdateBound();
    }
}
```
*(注意：直接修改 `BSTriShape` 的頂點在 SE 引擎中極度困難，因為頂點數據常常常駐在 GPU。更安全的做法是使用 NiTransform 縮放整個骨骼節點，或使用 Morph 動畫。)*

---

## 步驟四：強制刷新渲染
修改完成後，必須告訴引擎更新此物件。

```cpp
void CommitChanges(RE::NiNode* a_root) {
    if (!a_root) return;
    
    // 更新幾何變換與邊界
    RE::NiUpdateData updateData;
    updateData.time = 0.0f;
    updateData.flags = RE::NiUpdateData::Flag::kDirty;
    a_root->Update(updateData);
}
```

---

## 驗證與錯誤排除
1.  **貼圖變紫/黑**: 表示 `SetTexturePath` 給予的路徑錯誤，引擎找不到 `.dds` 檔案。
2.  **物件突然消失**: 修改頂點後未正確重新計算 `Bounding Box`，導致視錐剔除（Frustum Culling）誤判。
3.  **瞬間崩潰 (CTD)**: 你嘗試讀寫了處於 GPU 鎖定狀態的內存區塊。

---
*文件路徑：architectures/classified/3D_Graphics/Tutorials/Tutorial_Runtime_Mesh_and_Material_Modification.md*
