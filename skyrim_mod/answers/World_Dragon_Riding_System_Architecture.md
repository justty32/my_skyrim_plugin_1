# Skyrim 龍騎士系統技術架構 (Dragon Riding System)

原版的《龍裔》DLC 雖然加入了騎龍，但玩家只能控制「鎖定目標」而不能「自由移動」。要實作真正的龍騎士系統，需要將玩家與龍的行為圖進行深度連結。

---

## 1. 核心機制：骨架依附 (Mounting & Attachment)

### A. 乘客系統 (The Passenger Node)
- **原理**: 玩家不是「站」在龍背上，而是成為龍的一個「子節點」。
- **實作**:
    - 在龍的 NIF 中找到 `NPC Spine2` 節點。
    - 使用 `RE::NiNode::AttachChild()` 將玩家的根節點附加其上。
- **優點**: 龍在翻滾或俯衝時，玩家會完美地跟隨龍的骨骼同步擺動，不會發生位移。

### B. 玩家狀態切換
- **技術**: 進入騎乘狀態時，必須調用 `RE::PlayerCharacter::SetSitting(true)` 並進入特定的「騎龍動作 (DragonSit)」，否則玩家會以站姿穿透龍背。

---

## 2. 操控權接管 (Control Takeover)

這是讓龍「聽話」的關鍵。

### A. 攔截龍的 AI
- **原理**: 龍預設是由 AI Package 驅動的。
- **方案**: 
    1.  **暫停 AI**: 進入騎乘後，清除龍的所有 `CurrentPackage`。
    2.  **手動注入位移**: 
        ```cpp
        // 在 C++ 中根據玩家鏡頭方向直接修改龍的座標
        RE::NiPoint3 forwardVec = GetCameraForwardVector();
        float speed = 1500.0f; // 龍的飛行速度
        dragonActor->SetPosition(dragonActor->GetPosition() + (forwardVec * speed * deltaTime));
        ```

### B. 動畫圖同步 (Graph Sync)
- 玩家按下 `W` -> 傳送 `FlightStart` 事件給龍。
- 玩家按下 `滑鼠左鍵` -> 傳送 `DragonFireBreath` 事件。

---

## 3. 鏡頭優化 (Camera Overhaul)

騎乘巨龍時，預設的第三人稱相機通常會卡在龍的翅膀裡。
- **技術**: 使用 `RE::ThirdPersonState` 動態調整 `fOverShoulderPosX/Y`。
- **目標**: 將鏡頭焦點向後、向上大幅偏移，提供類似「空戰模擬」的廣闊視角。

---

## 4. 戰鬥與碰撞判定

### A. 噴火攻擊 (Breath Attack)
- 透過 C++ 動態獲取龍頭部的 `MouthNode` 座標。
- 使用 `RE::Projectile::Launch()` 手動從該點發射噴火投影物，方向與玩家準心同步。

### B. 物理衝突處理
- 龍在高空時應禁用與地面樹木的小型碰撞（避免卡頓），但保留與地表和建築的大型碰撞。

---

## 5. 核心類別原始碼標註

- **`RE::TESObjectREFR::SetVehicle`**: 引擎內部的依附接口。
- **`RE::CharacterProxy`**: 處理龍的物理膠囊體。
- **`RE::ActorValue`**: 修改 `SpeedMult` 影響飛行速度。
- **`RE::BSAnimationGraphManager`**: 同步玩家與龍的動作事件。

---
*文件路徑：architectures/classified/World/Answers/Dragon_Riding_System_Architecture.md*
