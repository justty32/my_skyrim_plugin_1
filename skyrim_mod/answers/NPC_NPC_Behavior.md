# Skyrim NPC 行為架構：AI 進程與行為邏輯

理解 NPC 的行為是如何驅動的，對於製作 AI 增強類或行為控制類模組至關重要。Skyrim 的 NPC 並非簡單的狀態機，而是一個複雜的分層決策系統。

---

## 1. AI 決策鏈 (The AI Chain)

NPC 的行為由以下核心組件共同驅動：
**`Actor` -> `AIProcess` -> `TESPackage` -> `BGSProcedureTree`**

### A. AIProcess：NPC 的大腦
- **原始碼**: `include/RE/A/AIProcess.h`
- 每個 `Actor` 都有一個 `currentProcess` 成員。它負責存儲 NPC 當前的感知數據、戰鬥狀態、路徑規劃以及正在執行的行為。
- **分層處理**: 引擎根據 NPC 與玩家的距離，將 AI 進程分為四個等級（High, Middle-High, Middle-Low, Low），距離越遠，運算頻率越低。

### B. TESPackage：行為藍圖
- **原始碼**: `include/RE/T/TESPackage.h`
- 這是行為的數據模板（Base Data）。定義了 NPC “在什麼時候、什麼地方、做什麼”。
- 例如：`Eat`, `Sleep`, `Patrol` (巡邏), `Sandboxing` (隨機互動)。
- 每個 Package 都有其優先級（Priority）和觸發條件（Conditions）。

### C. BGSProcedureTree：行為步驟
- **原始碼**: `include/RE/B/BGSProcedureTree.h`
- 這是現代化 Skyrim（從 Fallout 3 開始引入）使用的複雜行為樹。一個 `TESPackage` 內部可能包含一個過程樹，定義了具體的動作順序。

---

## 2. 感知與檢測系統 (Detection)

NPC 依靠感官來與世界交互。

- **Detection Level**: 標誌著 NPC 對玩家的發現程度（Hidden, Detecting, Alert, Lost）。
- **Detection Data**: 
    - **原始碼**: `include/RE/D/DetectionData.h`
    - 存儲了 NPC 的視覺範圍、聽覺閾值以及對特定目標的威脅評估。

---

## 3. 戰鬥系統 (Combat)

當 NPC 進入戰鬥狀態時，行為邏輯會從普通 Package 切換到戰鬥控制器。

### CombatController (戰鬥控制器)
- **原始碼**: `include/RE/C/CombatController.h`
- 它負責管理戰鬥中的決策：
    - **CombatBehavior**: 決定是衝鋒、躲避還是遠程射擊。
    - **CombatTarget**: 評估誰是最危險的目標。

---

## 4. 行為標誌與狀態 (Actor State)

NPC 的物理狀態會限制或觸發特定行為。

- **ActorState**: 
    - **原始碼**: `include/RE/A/ActorState.h`
    - 記錄了 NPC 是否正在潛行（Sneaking）、游泳、受傷或正在進行特定的動畫（Casting, Attacking）。

---

## 5. 開發實戰：如何影響 NPC 行為？

### 1. 強制執行新行為
如果你想讓一個 NPC 立刻逃跑或跪下：
- 使用 `Actor::EvaluatePackage()` 重新觸發行為評估。
- 通過 C++ 插件動態添加一個高優先級的 `TESPackage`。

### 2. 攔截 AI 更新
在 `AIProcess` 的更新循環中進行 Hook，可以實現如“時間停止時 NPC 凍結”或“修改 NPC 感知範圍”的功能。

### 3. 修改戰鬥屬性
通過操作 `CombatController`，你可以讓原本膽小的 NPC 變得極具攻擊性，或修改其尋找掩體的邏輯。

---

## 6. 行為架構圖

```text
Actor (實體)
  |
  +-- AIProcess (執行者: 處理路徑、感知、動畫)
        |
        +-- Current Package (目前藍圖: 定義目標地點、行為類型)
        |     |
        |     +-- Procedure Tree (具體動作序列: 走向椅子 -> 坐下 -> 播放吃飯動畫)
        |
        +-- Combat Controller (戰鬥大腦: 僅在戰鬥時接管)
              |
              +-- TargetSelector (選取目標)
              +-- BehaviorSelector (選取技能/戰術)
```
