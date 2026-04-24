# 實戰教學：新大地圖 (Worldspace) 建立開發指南 (New Worldspace)

本教學將引導你建立一個完全獨立的新區域（如索瑟姆島），包含地形生成、遠景渲染 (LOD) 以及與天際省的傳送門。

## 難度等級與準備工作
- **難度**: 極高 (Very Hard)
- **準備工具**:
    - **Creation Kit (CK)**: 核心開發工具。
    - **World Machine / L3DT**: 用於生成專業的高度圖 (Heightmap)。
    - **xLODGen / DynDOLOD**: 生成遠景 (LOD) 必備工具。

---

## 實作步驟

### 步驟一：定義 Worldspace
1. 在 CK 的 `World > Worldspace` 中點擊 New。
2. 設置 `Climate`（如：`SkyrimCloudy`）與 `Water`（海平面高度）。
3. 勾選 `No LOD Water` 以優化遠景效能。

### 步驟二：生成地形基底
1. 使用 `Heightmap Editing` 工具匯入你的高度圖 RAW 檔案。
2. 使用 `Landscaping` 工具 (快捷鍵 H) 繪製草地、雪地與岩石。
3. **重要**: 設置世界邊界 (World Boundary)，防止玩家走下地圖邊緣。

### 步驟三：設置導航與 AI
1. 為每個 Cell 手動或自動生成 Navmesh。
2. 點擊 `Finalize Navmesh` 確保相鄰單元間的邊界點已對齊。
3. 放置 `Map Marker` 以啟用快速旅行。

### 步驟四：遠景渲染 (LOD Generation)
1. 儲存模組並關閉 CK。
2. 運行 `xLODGen` 選擇你的 Worldspace，生成 Terrain LOD（地表遠景）。
3. 運行 `DynDOLOD` 生成 Object LOD（建築物與樹木遠景）。

---

## 代碼實踐 (C++ 環境檢測)

在插件中判斷玩家是否身處你的新世界，以便執行特定腳本（如禁用某些技能）：

```cpp
bool IsPlayerInMyNewWorld() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto currentWorld = player->GetWorldspace();
    
    if (currentWorld) {
        // 使用 EditorID 進行判斷（需先在 CK 中命名）
        std::string editorID = currentWorld->GetFormEditorID();
        if (editorID == "MyCustomWorldspaceID") {
            return true;
        }
    }
    return false;
}
```

---

## 常見問題與驗證
- **驗證方式**: 在天際省放置一扇門，傳送到新地圖，觀察遠處山脈是否可見。
- **問題 A**: 走一走掉進虛空？
    - *解決*: 這通常是因為該單元的 LOD 未生成或地形高度低於海平面。檢查 `uGridsToLoad` 設定。
- **問題 B**: NPC 跨越單元時卡住？
    - *解決*: 這是 Navmesh 斷裂。必須在 CK 中對相鄰兩個 Cell 執行 `Finalize Navmesh` 以焊接頂點。
- **性能提示**: 在新世界中使用 `5x5 Grid Loading` 原則佈置物體。不要在單一單元內塞入超過 1000 個動態物件。
