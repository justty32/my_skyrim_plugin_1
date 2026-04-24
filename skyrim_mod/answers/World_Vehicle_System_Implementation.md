# Skyrim 載具系統實作架構 (Vehicle System Implementation)

Skyrim 引擎原生並不支援傳統意義上的「載具」（如汽車、飛船）。要實作載具系統，通常需要對 `RE::Actor` 或 `RE::TESObjectREFR` 進行深度改造。

---

## 1. 實作方案對比

### 方案 A：基於 Actor 的改裝 (Actor-based / "Horse" Method)
- **原理**: 建立一個外觀為載具的 NPC（Race 設為載具模型），並利用馬匹的騎乘機制。
- **優點**: 實作簡單，原生支援玩家上車、AI 路徑尋找。
- **缺點**: 物理表現極差，無法模擬慣性、輪胎旋轉或真正的加速感。

### 方案 B：純物理驅動 (Physics-based / Havok Method)
- **原理**: 使用 `RE::bhkWorld` 在 C++ 中手動施加衝量 (Impulse) 與力 (Force)。
- **核心代碼**:
  ```cpp
  auto body = v_ref->GetPhysicsObject();
  if (body) {
      // 根據玩家輸入施加前進力
      RE::NiPoint3 force = { 0, 5000.0f, 0 }; 
      body->ApplyLinearImpulse(force);
  }
  ```
- **優點**: 真實的懸吊系統、碰撞與速度感。
- **缺點**: 需處理極其複雜的 Havok 座標變換與玩家依附（Attachment）問題。

### 方案 C：動畫導向 (Animation-based)
- **原理**: 載具是一個播放特定移動動畫的 `Static` 物件。
- **適用**: 開放世界中的馬車（Carriage）或固定航線的飛船。

---

## 2. 關鍵組件：玩家依附 (Passenger Attachment)

無論載具如何移動，玩家必須「黏」在上面。
- **技術細節**: 將玩家的 `Parent` 設置為載具的某個節點（Node），並禁用玩家的原生位移控制。
- **原始碼**: `RE::NiNode::AttachChild()`。

---

## 3. 操控系統 (Control Handling)

1.  **攔截輸入**: 使用 `RE::InputEvent` 攔截原生的 `W/A/S/D`。
2.  **狀態機**: 
    - **加速**: 逐漸增加力道。
    - **轉向**: 繞著垂直軸（Z-axis）施加扭矩（Torque）。
    - **剎車**: 增加物理摩擦係數或反向施力。

---

## 4. 技術挑戰

- **地形適應**: 載具必須根據下方的 Navmesh 或地形高度圖動態調整傾斜度（Pitch/Roll），否則會發生穿模。
- **加載邊界**: 載具移動速度若過快（超過 uGrids 加載速度），玩家會掉入未加載的空洞中。
- **碰撞傷害**: 需自定義邏輯，計算載具撞擊 NPC 時的動量並轉換為傷害。

---

## 5. 核心類別原始碼標註

- **`RE::bhkRigidBody`**: `include/RE/B/bhkRigidBody.h` (物理核心)
- **`RE::Actor::SetVehicle`**: 引擎內部預留的部分載具接口。
- **`RE::PlayerCharacter`**: 處理玩家騎乘狀態。

---
*文件路徑：architectures/classified/World/Vehicle_System_Implementation.md*
