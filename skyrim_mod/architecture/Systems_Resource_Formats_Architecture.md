# Skyrim 資源檔案系統架構：NIF, DDS, HKX 與 BSA 封裝

Skyrim 引擎（Gamebryo/Creation Engine）使用一套專有的二進制格式來處理 3D 數據、視覺與物理效果。理解這些格式的內部構造是製作高品質資源模組的基礎。

---

## 1. 檔案封裝：BSA (Bethesda Archive)
為了提高讀取效率，引擎不直接從磁盤讀取數萬個小文件，而是將它們打包。
- **機制**: BSA 本質上是一個帶有哈希索引的虛擬檔案系統。
- **原始碼**: `include/RE/B/BSResource/` (涉及 `RE::BSResource::LooseFileStream` 與 `RE::BSResource::Archive`)
- **優先級**: 「鬆散文件（Loose Files）」的讀取優先級高於 BSA。這就是為什麼你可以直接把模型扔進 `Data/Meshes` 來覆蓋遊戲原有的資源。

---

## 2. 3D 模型：NIF (NetImmerse Format)
這是引擎最核心的 3D 數據載體。
- **原始碼**: `include/RE/N/NiMain.h` (包含 `RE::NiNode`, `RE::NiTriShape`)
- **結構**: NIF 是一個「塊狀（Block-based）」結構。
    - **NiNode**: 骨骼節點，定義層級關係。
    - **NiTriShape**: 具體的幾何幾何數據（頂點、索引）。
    - **BSLightingShaderProperty**: 渲染指令，告訴引擎如何塗抹貼圖。
- **C++ 介入**: 透過 `RE::NiStream` 類別，你可以在插件中動態加載並解析 NIF 文件。

---

## 3. 貼圖與材質：DDS (DirectDraw Surface)
Skyrim 所有的圖片資源都是 DDS 格式，這是一種顯示卡可以直接讀取的格式。
- **Mipmaps**: DDS 內嵌了不同解析度的副本，引擎會根據距離動態切換，這就是為什麼遠處的牆壁看起來模糊（節省顯存）。
- **通道分配**:
    - **Diffuse**: 顏色數據。
    - **Normal**: 法線數據（決定物體表面的凹凸感），Alpha 通道通常儲存高光強度（Specular）。

---

## 4. 物理與動畫：HKX (Havok)
Skyrim 使用 Havok 引擎處理物理碰撞與骨骼動畫。
- **行為圖 (Behavior Graph)**: `.hkx` 文件定義了 NPC 何時切換動作（如：從「走路」轉到「跑步」的過渡）。
- **碰撞體 (Collision)**: 建築物的物理邊界也存儲在 NIF 內部的 Havok 數據塊中。

---

## 5. 音效與語音：FUZ, XWM, WAV
- **FUZ**: 專用語音格式。它將 **LIP (口型同步數據)** 與 **XWM (壓縮音訊)** 封裝在一起。
- **XWM**: Bethesda 特有的高壓縮比音訊格式，適合背景音樂。
- **WAV**: 無損格式，通常用於短促的特效音（如揮劍聲）。

---

## 6. C++ 插件開發中的資源讀取

在 C++ 中，你通常不直接操作二進制文件，而是透過引擎提供的流接口：

```cpp
void LoadCustomModel(const char* a_path) {
    // 透過 BSResource 系統查找資源 (BSA 或 Loose)
    auto resourceEntry = RE::BSResource::EntryDB::GetSingleton();
    
    // 使用 NiStream 加載 NIF
    RE::NiStream nifStream;
    if (nifStream.Load(a_path)) {
        auto rootNode = nifStream.GetObjectAt<RE::NiNode>(0);
        // 現在你可以將這個模型掛載到玩家手上或世界中
    }
}
```

---

## 7. 核心類別原始碼標註

- **`RE::BSResource`**: `include/RE/B/BSResource.h` - 資源加載系統基石。
- **`RE::NiStream`**: `include/RE/N/NiStream.h` - NIF 解析流。
- **`RE::NiAVObject`**: `include/RE/N/NiAVObject.h` - 所有可渲染 3D 對象的基類。
- **`RE::BSShaderProperty`**: `include/RE/B/BSShaderProperty.h` - 材質渲染屬性。
