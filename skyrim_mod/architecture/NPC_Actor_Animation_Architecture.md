# Skyrim Actor 動作與動畫架構：Havok Behavior 與動畫事件

Skyrim 的動畫系統並非簡單地播放「影片剪輯」，而是一個基於 **Havok Behavior** 的複雜狀態機系統。它決定了角色何時從「行走」切換到「跑步」，以及攻擊動作何時造成傷害。

---

## 1. 核心組件：動畫管理器 (Animation Graph Manager)

每個 Actor 都有一個動畫圖管理器，負責與 Havok 引擎通訊。
- **接口**: `RE::IAnimationGraphManagerHolder`
- **原始碼**: `include/RE/I/IAnimationGraphManagerHolder.h`
- **職責**: 它是 C++ 代碼與底層 `.hkx` 行為圖文件之間的橋樑。

---

## 2. 行為圖 (Behavior Graphs - .hkx)

動畫的邏輯存儲在 `Data\Meshes\Actors\Character\Behaviors\` 下的 `.hkx` 文件中。
- **狀態機 (State Machine)**: 定義了動作的過渡邏輯。例如：只有在 `IsSprinting` 變量為真時，才能進入「衝刺」狀態。
- **混合 (Blending)**: 允許同時播放多個動畫。例如：上半身播放「揮劍」，下半身播放「走路」。

---

## 3. 動畫事件 (Animation Events)：動作的語言

這是開發者最常接觸的部分。事件是觸發動作或由動作觸發的「信號」。
- **常見事件**:
    - `AttackStart`: 開始攻擊。
    - `HitFrame`: 武器揮到傷害判定點（在此幀計算傷害）。
    - `JumpUp`: 跳躍起跳。
- **C++ 觸發方式**:
    ```cpp
    // 強迫 Actor 播放一個特定的動畫片段
    a_actor->NotifyAnimationGraph("ShoutStart");
    ```

---

## 4. 動畫變量 (Animation Variables)

狀態機的切換依賴於「變量」。
- **類型**: Float（如 `Speed`）, Bool（如 `IsSneaking`）, Int。
- **C++ 操作**: 你可以透過 C++ 強行修改這些變量來改變 NPC 的動作。
    ```cpp
    float currentSpeed;
    a_actor->GetGraphVariableFloat("Speed", currentSpeed);
    a_actor->SetGraphVariableBool("IsBlocking", true);
    ```

---

## 5. 逆向運動學 (IK - Inverse Kinematics)

Skyrim 使用 IK 技術來處理地形交互：
- **FootIK**: 確保 NPC 的腳掌能平貼在斜坡上，而不是懸空。
- **LookIK**: 讓 NPC 的頭部動態轉向玩家或目標，這涉及到對 `NPC Head` 骨骼節點的實時旋轉補算。

---

## 6. C++ 插件開發中的高級控制

### A. 監聽動畫事件
你可以 Hook `RE::BSTEventSink<RE::BSAnimationGraphEvent>`，在每一幀獲取 Actor 剛剛發出了什麼信號（如：監聽 `HitFrame` 實現精確格擋）。

### B. 動作攔截
透過 Hook `RE::IAnimationGraphManagerHolder::NotifyAnimationGraph`，你可以攔截並取消某些動作（如：在特定狀態下禁止跳躍）。

---

## 7. 核心類別原始碼標註

- **`RE::IAnimationGraphManagerHolder`**: `include/RE/I/IAnimationGraphManagerHolder.h` - 動畫操控接口。
- **`RE::BSAnimationGraphManager`**: `include/RE/B/BSAnimationGraphManager.h` - 運行時管理器。
- **`RE::BSAnimationGraphEvent`**: `include/RE/B/BSAnimationGraphEvent.h` - 動畫事件數據。
- **`RE::hkbCharacter`**: `include/RE/H/hkbCharacter.h` - Havok 物理角色底層。

---

## 8. 技術總結
1.  **數據層**: `.hkx` 文件定義了動作的邏輯與過渡。
2.  **變量層**: `Speed`, `IsSneaking` 等決定了狀態機的走向。
3.  **事件層**: `NotifyAnimationGraph` 是發送指令的手段。
4.  **物理層**: Havok 處理骨骼的混合與地形適配。
 village
