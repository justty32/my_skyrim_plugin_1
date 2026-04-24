# Skyrim 渲染架構：Shader、貼圖與動態覆蓋機制

如果 Mesh 是「骨骼與肌肉」，那麼 Shader（著色器）就是「皮膚與化妝」。

---

## 1. 核心組件：`BSLightingShaderProperty`
這是 Skyrim 視覺效果的「大腦」。它決定了 Mesh 應該如何反射光線。
- **原始碼**: `include/RE/B/BSLightingShaderProperty.h`
- **功能**: 它連結了幾何體（Mesh）與具體的貼圖文件（DDS）。

---

## 2. 貼圖的三位一體：Diffuse, Normal, Specular
在 `BSShaderTextureSet` 中，一個完整的「起司外觀」由三張圖組成：
1.  **Diffuse (顏色圖)**: 起司的黃色外觀。
2.  **Normal (法線圖)**: 模擬起司表面的凹凸小孔。
3.  **Specular (高光圖)**: 決定起司看起來是油膩的發亮，還是乾巴巴的啞光。

---

## 3. 動態材質覆蓋的底層：`ExtraTextureSet`
在教程 27 中，我們使用了這個類別。
- **機制**: 引擎在渲染每個實體前，會檢查它的 `ExtraDataList`。
- **邏輯**: 如果發現裡面有 `ExtraTextureSet`，引擎會**臨時屏蔽** NIF 模型文件裡寫死的貼圖路徑，強行讓顯示卡去讀取你指定的「起司貼圖集」。
- **優點**: 這種操作不需要修改磁盤上的模型文件，非常安全。

---

## 4. 特效渲染：Emissive & Alpha
為什麼火球會發光？為什麼玻璃是透明的？
- **Emissive Color (自發光)**: 這是 Shader 中的一個屬性。當你讓武器燃起火焰時，我們其實是在修改 Shader 的 `emissiveColor`，讓它即使在晚上也像燈泡一樣亮。
- **Alpha Testing**: 決定哪些部分是透明的。如果你想讓起司中間有個洞，你需要操作貼圖的 Alpha 通道。

---

## 5. C++ 實戰技巧：尋找特定網格
一個模型（如玩家）有幾十個網格。你只想把「劍」變起司，不想把「玩家」變起司：
```cpp
void TargetSpecificMesh(RE::NiAVObject* a_root) {
    // 使用 Visit 工具遍歷
    RE::BSVisit::TraverseScenegraphGeometries(a_root, [](RE::BSTriShape* a_mesh) {
        // 檢查網格名稱，例如 "Sword"
        if (a_mesh->name.contains("Sword")) {
            // 只對這個網格施加材質覆蓋
        }
        return RE::BSVisit::BSVisitControl::kContinue;
    });
}
```

---

## 6. 核心類別原始碼標註
- **`RE::BSLightingShaderProperty`**: `include/RE/B/BSLightingShaderProperty.h` - 渲染控制。
- **`RE::BSShaderTextureSet`**: `include/RE/B/BSShaderTextureSet.h` - 貼圖路徑清單。
- **`RE::ExtraTextureSet`**: `include/RE/E/ExtraTextureSet.h` - 動態材質覆蓋器。
- **`RE::BSVisit`**: `include/RE/B/BSVisit.h` - 遍歷模型樹的工具。
