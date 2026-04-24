# Skyrim 深層 AI 架構：接管 NPC 的決策中樞

當 `TESPackage` 無法滿足你的模組需求時，你需要深入引擎的 `AIProcess` 與 `CombatController` 層面。這涉及到直接修改引擎的運行時決策。

---

## 1. 決策中樞：`AIProcess`
每個 Actor 都有一個 `AIProcess` 對象，它是 NPC 的「運行內存」。
- **原始碼**: `include/RE/A/AIProcess.h`
- **核心函數**: `Update()`。
- **機制**: 引擎每一幀（或根據等級間隔）調用 `Update`。在該函數中，NPC 會檢查周邊環境、更新目標坐標、並判斷是否需要切換動作。
- **Hook 價值**: 通過 Hook `Update`，你可以在 NPC 思考之前改變他的感知，或者在 NPC 移動之前強行改變他的目標。

---

## 2. 戰鬥大腦：`CombatController`
一旦 NPC 進入戰鬥，決策權就移交給了戰鬥控制器。
- **原始碼**: `include/RE/C/CombatController.h`
- **組成**:
    - **`CombatTargetSelector`**: 決定誰是下一個目標。
    - **`CombatBehaviorController`**: 決定下一個動作（防禦、重擊、後撤）。
- **Hook 價值**: 這是製作「戰鬥 AI 增強」模組的核心。你可以 Hook `PickNextBehavior` 來強迫 NPC 在低血量時執行特定的逃跑或求援邏輯。

---

## 3. 行為樹：`BGSProcedureTree`
這是 Package 內部的具體執行步驟。
- **原始碼**: `include/RE/B/BGSProcedureTree.h`
- **機制**: 定義了諸如「移動到某處」、「播放動畫」、「等待」等原子操作。
- **深層修改**: 你可以透過 C++ 動態修改 NPC 當前正在執行的 Procedure，實現極其精細的動作控制。

---

## 4. 尋路與路徑：`Pathing`
NPC 是如何計算 A* 尋路的？
- **原始碼**: `include/RE/P/Pathing.h`
- **機制**: 引擎根據 Navmesh 計算一條路徑點（Path Points）隊列。
- **Hook 價值**: 你可以攔截路徑計算，讓 NPC 故意避開某些區域（如玩家佈下的陷阱區），或者引導 NPC 走一條更具策略性的路線。

---

## 5. 技術總結：如何實施深層修改？

1.  **Thunking / Hooking**: 使用 `REL::Relocation` 找到對應的虛函數地址。
2.  **State Manipulation**: 在 `Update` 循環中直接修改 `ActorState` 或 `AIProcess` 內部的私有變量。
3.  **Event Injection**: 透過 `NotifyAnimationGraph` 手動觸發行為轉換。

## 6. 核心類別原始碼標註

- **`RE::AIProcess`**: `include/RE/A/AIProcess.h` - AI 運行時。
- **`RE::CombatController`**: `include/RE/C/CombatController.h` - 戰鬥決策。
- **`RE::Pathing`**: `include/RE/P/Pathing.h` - 尋路邏輯。
