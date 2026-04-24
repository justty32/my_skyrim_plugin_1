# 實戰教學：可互動家具與樣式更換系統 (Interactive Furniture & Styles)

本教學將教你如何製作具備自定義互動功能的家具，並實作在遊戲中即時切換家具「外觀樣式」的功能。

## 難度等級與準備工作
- **難度**: 中階 (Medium)
- **準備工具**:
    - **NifSkope**: 用於檢查家具的 `Furniture Marker`。
    - **Creation Kit (CK)**: 定義 `BGSTextureSet`。
    - **CommonLibSSE-NG**: 實作動態貼圖切換。

---

## 實作步驟

### 步驟一：建立具備 Marker 的家具
1. 在 NifSkope 中打開家具模型，確保其包含 `BSFurnitureMarker` 節點。
2. 在 CK 中建立一個 `Furniture` Form，並指定對應的模型。
3. 設置互動標籤（如：`Sit` 或 `Sleep`），這決定了玩家點擊時播放的動畫。

### 步驟二：定義不同的貼圖樣式
1. 在 CK 的 `Miscellaneous > Texture Set` 中建立多個項目。
2. 為每個項目分配不同的貼圖路徑（如：木紋、石紋、皮紋）。

### 步驟三：實作樣式切換邏輯 (C++)
1. 獲取家具的 `RE::NiNode` 渲染節點。
2. 遍歷節點中的 `BSGeometry` 或 `NiTriShape`。
3. 修改其 `BSLightingShaderProperty` 的 `Material` 或直接更換 `TextureSet`。
4. 調用 `Update3DModel()` 重新渲染。

### 步驟四：樣式學習系統 (Blueprints)
1. 建立一個 `GlobalVariable` 或使用 SKSE 保存一個已解鎖樣式的清單。
2. 玩家在閱讀特定書籍（設計圖）後，將對應樣式標記為「已學習」。

---

## 代碼實踐 (C++ - CommonLibSSE-NG)

以下為動態更換物件貼圖的核心範例：

```cpp
void ApplyTextureSet(RE::TESObjectREFR* a_ref, RE::BGSTextureSet* a_txSet) {
    if (!a_ref || !a_txSet) return;

    auto node = a_ref->Get3D();
    if (node) {
        // 訪問底層 3D 屬性
        auto shaderProp = node->AsNode()->GetFirstGeometry()->GetShaderProperty();
        auto lightingProp = netimmerse_cast<RE::BSLightingShaderProperty*>(shaderProp);
        
        if (lightingProp) {
            // 更換紋理集 (Texture Set)
            lightingProp->SetTextureSet(a_txSet);
            
            // 強制重新加載渲染數據
            a_ref->Update3DModel();
            RE::ConsoleLog::GetSingleton()->Print("家具樣式已更新！");
        }
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 在遊戲中對準一張椅子按下快捷鍵，檢查其顏色是否在「木質」與「石質」間切換。
- **問題 A**: 互動時動畫不對？
    - *解決*: 檢查 `Furniture Marker` 的方向，如果 Marker 朝向牆壁，玩家會穿牆坐下。
- **問題 B**: 貼圖切換後看起來黑黑的？
    - *解決*: 確保新的 `TextureSet` 包含正確的法線貼圖 (Normal Map)，否則光影會出錯。
