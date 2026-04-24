# Monster Hunter 風格戰鬥系統技術架構 (Combat Overhaul)

要將 Skyrim 的戰鬥系統改造為類似 Monster Hunter (MH)，需要從「動作驅動」、「物理反饋」與「魔物交互」三個層次進行深度重構。

---

## 1. 核心支柱：動作驅動戰鬥 (Animation-Driven)

### A. 攻擊路徑與位移 (Root Motion)
- **問題**: Skyrim 原生攻擊是滑動的，缺乏重量感。
- **方案**: 使用 **MCO (Modern Combat Overhaul)** 框架的概念。透過 SKSE 讓攻擊動作完全由動畫的 `Root Motion` 驅動。每一擊的位移距離與方向都寫死在動畫檔 (.hkx) 中，消除「溜冰感」。

### B. 連段系統 (Combo System)
- **技術**: 利用 `Animation Event` 標籤。
- **實作**: 在動畫圖中設置 `WeaponSwing` 與 `ComboWindow` 事件。C++ 插件監聽玩家在特定時間視窗內的輸入，決定下一個播放的動畫節點。

---

## 2. 物理反饋：打擊感 (The "Weight")

### A. 卡肉與震動 (Hit-stop / Hit-lag)
- **原理**: 當武器命中敵人時，暫停攻擊者與受擊者的動畫數幀。
- **實作**:
  ```cpp
  // 命中時暫停時間軸
  void ApplyHitStop(RE::Actor* a_actor, float a_duration) {
      auto process = a_actor->GetActorRuntimeData().currentProcess;
      if (process && process->middleHigh) {
          // 暫停動畫更新
          a_actor->GetHostileAnimationGraph()->SetSpeed(0.01f);
          // 延時後恢復
          ScheduleTask(a_duration, [=](){ a_actor->GetHostileAnimationGraph()->SetSpeed(1.0f); });
      }
  }
  ```

### B. 鏡頭震動 (Screen Shake)
- **技術**: 呼叫 `RE::PlayerCamera::StartCameraShake()`，震動強度應與武器類型（如大劍 > 片手劍）掛鉤。

---

## 3. 魔物交互：部位破壞與硬直

### A. 定點傷害系統 (Locational Damage)
- **技術**: 攔截 `RE::Projectile::HandleHit` 或 `RE::Actor::HandleHit`。
- **邏輯**: 透過 `RE::NiAVObject` 獲取命中點的節點名稱（如 `NPC Head`, `NPC L Wing`）。
- **破壞機制**: 為特定部位設置隱藏的「部位生命值」。當該數值歸零，觸發特效（火花、斷裂模型切換）並強制魔物進入長硬直 (Stagger)。

### B. 魔物 AI 行為
- **疲勞度系統**: 為魔物建立一個隨攻擊降低的 `Stamina` 變數。當歸零時，強制切換至 `Exhausted` 動畫狀態。
- **預兆動作 (Telegraphing)**: 修改魔物的 `Behavior Graph`，確保強大攻擊前有明顯的起手勢，讓玩家能進行「精確閃避」。

---

## 4. 核心類別原始碼標註

- **`RE::Actor::HandleHit`**: 攔截傷害與打擊反饋。
- **`RE::IAnimationGraphManager`**: 控制動畫播放速度與狀態切換。
- **`RE::bhkWorld`**: 用於精確的物理碰撞檢測（部位判定）。

---
*文件路徑：architectures/classified/Systems/Answers/Monster_Hunter_Combat_Overhaul.md*
