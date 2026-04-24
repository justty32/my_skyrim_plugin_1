# Skyrim 建築物內部改造與室內空間設計 (Interior Renovation)

改造一個現有的建築物內部（Interior Cell），在技術上涉及空間容器（Cell）的修改、物件引用（Reference）的處理以及導航網格（Navmesh）的重新封裝。

---

## 1. 核心概念：`RE::TESObjectCELL`

室內空間在引擎中被視為一個獨立的容器。
- **座標系統**: 室內單元擁有獨立的座標系，通常以 `(0, 0, 0)` 為中心。
- **屬性**: 包含了照明模板 (Lighting Template)、環境音效 (Acoustic Space) 以及是否允許等待/快速旅行的標籤。

---

## 2. 靜態改造路徑 (Creation Kit / ESP)

這是最常見的做法，直接修改遊戲的原始數據。

### A. 物件處理規則
- **不要直接刪除 (Delete)**: 如果你直接刪除原版的物件（如一根柱子），可能會導致其他依賴該物件的模組發生崩潰。
- **建議做法**: 將物件移動到地圖下方（Z 軸 -3000），或者勾選 `Initially Disabled`。

### B. 導航網格 (Navmesh) 的修正
如果你在地板上新增了一個大型櫃子，NPC 會嘗試穿過它。
- **必須操作**: 重新繪製受影響區域的 Navmesh。
- **邊界檢查**: 確保門口的 `Teleport Marker` 周圍有足夠的導航空間，否則 NPC 進門後會卡在原地。

---

## 3. 動態改造路徑 (Dynamic Renovation)

如果你希望建築物內部能隨著劇情（例如：玩家花錢修理破舊的房屋）而改變，你需要使用動態控制。

### A. 使用 XMarker 進行批次控制
1.  **分組**: 將所有「破舊家具」連結到一個名為 `OldStuffMarker` 的父物件。
2.  **替換**: 將所有「新家具」連結到一個 `NewStuffMarker`。
3.  **邏輯**:
    ```cpp
    // 透過 C++ 或腳本切換裝潢狀態
    void UpgradeHouse(RE::TESObjectREFR* a_oldMarker, RE::TESObjectREFR* a_newMarker) {
        a_oldMarker->Disable(); // 隱藏所有舊東西
        a_newMarker->Enable();  // 顯示所有新東西
    }
    ```

### B. 動態物件生成 (`PlaceAtMe`)
- 可以在運行時動態將家具放置到特定座標，並將其引用保存到 SKSE 存檔中（參考 `Custom_Housing_and_Furniture_System.md`）。

---

## 4. 效能與視覺優化

### A. Room Bounds & Portals (關鍵！)
大型建築（如龍臨堡）若不優化會非常卡。
- **Room Bound**: 建立一個看不見的盒子，將物件分配給它。只有當玩家在盒子內（或看得到盒子）時，內部的東西才會被渲染。
- **Portal**: 盒子與盒子之間的「窗戶」，定義了光線與視線的傳遞路徑。

### B. 照明模板 (Lighting Template)
- **Ambient**: 環境光顏色。
- **Directional**: 模擬窗戶射入的陽光。
- **Fog**: 室內霧氣深度，能有效增加空間感。

---

## 5. 技術挑戰：光影閃爍 (Flickering Lights)
Skyrim 引擎限制一個網格表面同時只能受到 **4 個動態光源** 的照射。
- **現象**: 如果你放了太多蠟燭，轉動視角時燈光會突然消失又出現。
- **解決方案**: 
    1. 使用 `Static` 光源（不產生陰影）。
    2. 確保光源的照射範圍 (Radius) 不要互相重疊過多。

---

## 6. 核心類別原始碼標註

- **`RE::TESObjectCELL`**: `include/RE/T/TESObjectCELL.h`
- **`RE::BGSLightingTemplate`**: 照明設置。
- **`RE::TESObjectREFR`**: 所有的裝飾與家具實體。

---
*文件路徑：architectures/classified/World/Interior_Renovation_Architecture.md*
