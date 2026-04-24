# Skyrim 貼圖「三位一體」架構：DDS 格式、資源路徑與動態操控

在 Skyrim 中，一個物體的表面質感是由一組（通常為三張）相互配合的貼圖決定的。理解這三者的協作關係，是操控視覺效果的核心。

---

## 1. 貼圖的三個核心組成

### A. Diffuse Map (顏色貼圖 / _d.dds)
- **作用**: 定義物體的基本顏色和花紋（如：起司的黃色）。
- **Alpha 通道**: 通常儲存透明度（Transparency）。

### B. Normal Map (法線貼圖 / _n.dds)
- **作用**: 模擬表面的凹凸細節（如：起司表面的小孔）。
- **原理**: 透過顏色（RGB）告訴引擎光線照射時的傾斜角度，從而產生虛擬的陰影。
- **Alpha 通道**: **極其關鍵！** 在 Skyrim 中，法線貼圖的 Alpha 通道儲存的是 **Specular (高光強度)**。

### C. Specular/Glow Map (高光與自發光 / _s.dds 或 _g.dds)
- **Specular (_s.dds)**: 決定光澤度（金屬感 vs 啞光）。
- **Glow (_g.dds)**: 定義哪些部位在黑暗中會發光（如魔法符文）。

---

## 2. 檔案格式與資源位置

### A. 檔案格式：DDS (DirectDraw Surface)
- **特徵**: 顯示卡直接支持的壓縮格式（BC1, BC3, BC7）。
- **Mipmaps**: 內建多個解析度級別，引擎根據距離自動切換以節省顯存。

### B. 資源所在
- **路徑**: 存儲在 `Data\Textures\` 目錄下（Loose Files）或 `Skyrim - Textures.bsa` 中。
- **命名約定**:
    - `example.dds` (Diffuse)
    - `example_n.dds` (Normal)
    - `example_msn.dds` (FaceGen 專用法線)

---

## 3. 如何透過 C++ 插件操作貼圖

在插件中，貼圖信息儲存在 `RE::BSShaderTextureSet` 類別中。

### A. 獲取當前貼圖
```cpp
auto shaderProp = geometry->GetShaderProperty()->As<RE::BSLightingShaderProperty>();
if (shaderProp) {
    auto textureSet = shaderProp->textureSet.get();
    const char* diffusePath = textureSet->GetTexturePath(RE::BSTextureSet::Texture::kDiffuse);
}
```

### B. 動態更換貼圖 (核心技術)
你可以建立一個新的 `BSShaderTextureSet` 並強行賦值。

```cpp
void ChangeTexture(RE::BSTriShape* a_mesh, const char* a_newDdsPath) {
    // 1. 創建新的貼圖集 (include/RE/B/BSShaderTextureSet.h)
    auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BSShaderTextureSet>();
    auto newSet = factory->Create()->As<RE::BSShaderTextureSet>();

    // 2. 設置新路徑 (例如：將 Diffuse 換成起司貼圖)
    newSet->SetTexturePath(RE::BSTextureSet::Texture::kDiffuse, a_newDdsPath);
    
    // 3. 應用到渲染屬性
    auto shaderProp = a_mesh->GetShaderProperty()->As<RE::BSLightingShaderProperty>();
    if (shaderProp) {
        shaderProp->textureSet.reset(newSet);
    }
}
```

---

## 4. 核心類別原始碼標註

- **`RE::BSShaderTextureSet`**: `include/RE/B/BSShaderTextureSet.h` - 貼圖路徑的容器。
- **`RE::BSTextureSet::Texture`**: `include/RE/B/BSTextureSet.h` - 貼圖類型枚舉（Diffuse, Normal, etc.）。
- **`RE::BSLightingShaderProperty`**: `include/RE/B/BSLightingShaderProperty.h` - 渲染屬性控制。
- **`RE::NiTexture`**: `include/RE/N/NiTexture.h` - 內存中的貼圖對象。
 village
---

## 5. 技術總結：開發者的視角
1.  **想改顏色**：換 `_d.dds`。
2.  **想改凹凸感**：換 `_n.dds`。
3.  **想改金屬反光**：修改 `_n.dds` 的 **Alpha 通道**。
4.  **想動態生效**：操作 `BSShaderTextureSet` 並調用 `Update3DModel()`。
