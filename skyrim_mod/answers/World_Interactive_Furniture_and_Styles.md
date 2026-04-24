# Skyrim 可互動家具與樣式更換系統 (Interactive Furniture & Styles)

本系統探討如何實作「可被玩家操作」的家具，以及如何在遊戲中「動態更換家具樣式」（例如更換沙發貼圖或地毯花紋）。

---

## 1. 家具互動邏輯 (Interaction)

### A. 坐下與睡覺
- **原理**: 家具 NIF 檔案中包含特定的 **Furniture Markers (互動點)**。
- **技術細節**: 當玩家點擊家具時，引擎會查找最近的 Marker 並播放對應的動畫（如 `SitStart`）。
- **自定義動作**: 透過 `RE::TESFurniture` 類別，你可以指定自定義的動畫標籤（Animation Tags），讓玩家在使用你的實驗桌時播放「調配藥劑」的動作而非單純坐下。

### B. 觸發式家具 (Toggleable Furniture)
- **實作**: 
    1. 給予家具一個腳本或 C++ 攔截。
    2. 當玩家互動時，切換家具的 `Animation State`（例如：打開折疊桌）。

---

## 2. 動態樣式更換 (Dynamic Styling)

如果你希望玩家能點擊一張椅子並將其從「木頭材質」切換為「石質材質」，有兩種技術路徑：

### 方案 A：紋理集切換 (TextureSet Swapping)
- **原理**: 不更換模型，僅更換貼圖。
- **實作**:
  ```cpp
  void ChangeStyle(RE::TESObjectREFR* a_ref, RE::BGSTextureSet* a_newTexture) {
      auto node = a_ref->Get3D();
      if (node) {
          // 遍歷所有 NiTriShape，並修改其 BSLightingShaderProperty 的紋理數據
          UpdateNodeTexture(node, a_newTexture);
          a_ref->Update3DModel();
      }
  }
  ```

### 方案 B：模型替換 (Model Swapping)
- **原理**: 直接將物件替換為另一個 Form。
- **缺點**: 會導致物件 ID (Handle) 變更，可能影響腳本連結。
- **優點**: 樣式可以有完全不同的幾何形狀。

---

## 3. 學習與解鎖機制 (Learning/Blueprints)

這回應了您提到的「可以學習」的概念。
- **技術實作**: 
    1. 建立一個 `GlobalVariable` 或在 C++ 中維護一個 `std::set<FormID> learnedStyles`。
    2. 玩家閱讀「家具設計圖」物品時，將對應 ID 加入已學習列表。
    3. 在建設 UI 中，根據該列表過濾可顯示的樣式。

---

## 4. 核心類別原始碼標註

- **`RE::TESFurniture`**: `include/RE/T/TESFurniture.h` - 互動定義。
- **`RE::BGSTextureSet`**: `include/RE/B/BGSTextureSet.h` - 紋理集。
- **`RE::NiNode`**: 底層 3D 節點操作。

---
*文件路徑：architectures/classified/World/Interactive_Furniture_and_Styles.md*
