# 實戰教學：高階模組化鍛造系統 (Advanced Modular Crafting)

本教學將教你如何突破 Skyrim 的單一模型限制，建立一個讓玩家能自定義「劍身」與「劍柄」的動態武器鍛造系統。

## 難度等級與準備工作
- **難度**: 極高 (Very Hard)
- **準備工具**:
    - **NifSkope**: 用於在模型中建立 Node Markers。
    - **CommonLibSSE-NG**: 用於動態合併 3D 模型與修改傷害數值。
    - **Flash/ActionScript 3**: 用於製作自定義鍛造選單。

---

## 實作步驟

### 步驟一：準備模組化 NIF 模型
1. 在基礎武器模型（如 `BaseSword.nif`）中，建立兩個空的 `NiNode`，命名為 `BladeMarker` 與 `HiltMarker`。
2. 將劍身與劍柄分別匯出為獨立的 NIF 檔案，確保其原點與 Marker 位置對齊。

### 步驟二：建立組件數據庫
1. 在 C++ 插件中定義組件屬性（如傷害加成、重量、模型路徑）。
2. 使用 `FormList` 或 JSON 檔案來儲存所有可選的劍身與劍柄數據。

### 步驟三：動態 3D 合併邏輯
1. 攔截武器的裝備事件或 3D 加載事件。
2. 使用 `RE::NiStream` 加載選定的組件 NIF。
3. 將組件作為子節點掛載到 `BladeMarker` 與 `HiltMarker` 下。

### 步驟四：動態數值 Hook
1. Hook `RE::TESObjectWEAP::GetDamage()`。
2. 從該武器實例的 `ExtraDataList` 讀取組件 ID。
3. 根據組件計算最終傷害並回傳給引擎。

---

## 代碼實踐 (C++ - CommonLibSSE-NG)

以下為動態掛載模型的關鍵邏輯：

```cpp
void AttachWeaponComponent(RE::NiNode* a_weaponRoot, const char* a_markerName, const char* a_nifPath) {
    auto marker = a_weaponRoot->GetObjectByName(a_markerName);
    if (!marker) return;

    // 加載新的 NIF
    RE::NiStream nifStream;
    if (nifStream.Load(a_nifPath)) {
        auto newPart = nifStream.GetObjectAt(0)->AsNode();
        
        // 移除舊組件（如果有）
        auto markerNode = marker->AsNode();
        markerNode->DetachAllChildren();
        
        // 附加新組件
        markerNode->AttachChild(newPart, true);
        
        // 更新矩陣與渲染
        a_weaponRoot->Update(RE::NiUpdateData{});
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 進入遊戲後透過控制台命令更換組件，觀察武器外觀是否即時改變。
- **問題 A**: 模型位置偏移？
    - *解決*: 在 NifSkope 中檢查 `BladeMarker` 的座標，確保其相對於 `BaseSword` 的位置正確。
- **問題 B**: 存檔後外觀重設？
    - *解決*: 必須在 `SKSE Co-save` 中保存組件數據，並在 `kLoadGame` 時重新執行掛載邏輯。
