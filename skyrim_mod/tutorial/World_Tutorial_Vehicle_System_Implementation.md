# 實戰教學：載具系統實作指南 (Vehicle System)

本教學將探討如何在 Skyrim 引擎中實作「可操控載具」，包含物理驅動與動畫驅動兩種方案。

## 難度等級與準備工作
- **難度**: 極高 (Very Hard)
- **準備工具**:
    - **CommonLibSSE-NG**: 處理底層 Havok 物理衝量。
    - **NifSkope**: 修改模型碰撞層 (Collision Layer)。
    - **Blender/3ds Max**: 製作載具動畫。

---

## 實作步驟

### 步驟一：選擇核心方案
- **方案 A (馬匹修改)**: 最快但不真實。修改 `HorseRace` 的模型。
- **方案 B (物理衝量)**: 最真實但難度最高。手動對 `RE::bhkRigidBody` 施加力道。

### 步驟二：實作玩家依附 (Attachment)
1. 當玩家點擊載具時，調用 `RE::Actor::SetVehicle()`。
2. 將玩家的位移權限移交給載具。
3. 播放「駕駛」動畫（如：手扶方向盤）。

### 步驟三：操控邏輯與力學 (C++)
1. 攔截 `W/A/S/D` 輸入。
2. **加速**: 根據按住時間增加 `LinearImpulse`。
3. **轉向**: 增加 `AngularImpulse` (角衝量) 實現旋轉。

### 步驟四：地形適應與物理優化
1. 使用射線檢測 (Raycast) 測量載具四個角落與地面的距離。
2. 自動調整載具的 `Pitch` (俯仰) 與 `Roll` (橫滾)，使其貼合坡道。

---

## 代碼實踐 (C++ 物理推動力範例)

```cpp
void MoveVehicle(RE::TESObjectREFR* a_vehicle, float a_forwardAmount) {
    auto bhkBody = a_vehicle->GetPhysicsObject();
    if (bhkBody) {
        auto rigidBody = static_cast<RE::bhkRigidBody*>(bhkBody->referencedObject.get());
        
        // 獲取前進方向向量
        RE::NiPoint3 forwardVec = GetForwardVector(a_vehicle);
        RE::NiPoint3 impulse = forwardVec * a_forwardAmount * 1000.0f;

        // 施加力道
        rigidBody->ApplyLinearImpulse(impulse);
        
        // 處理地形碰撞...
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 在控制台生成載具，嘗試按 W 前進，觀察載具是否有「慣性」滑行。
- **問題 A**: 載具撞到石頭會飛天？
    - *解決*: 這是 Havok 物理的常見問題。需調低載具的重心 (Center of Mass) 並增加重量屬性。
- **問題 B**: 玩家在載具移動時「滑下去」？
    - *解決*: 確保玩家已正確 `Attach` 到載具的 Node 上，且禁用了玩家的腳步移動 (`DisablePlayerControls`)。
