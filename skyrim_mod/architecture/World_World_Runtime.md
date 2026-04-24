# Skyrim 世界運行機制：主循環、離線 AI 與事件驅動

理解 Skyrim 引擎如何在每一幀（Tick）中管理成千上萬的對象，以及如何處理那些玩家看不見的 NPC，是開發大規模系統模組的關鍵。

---

## 1. 主循環 (The Main Loop / Tick)

Skyrim 的核心運行邏輯封裝在 `RE::Main` 類別中。

- **原始碼**: `include/RE/M/Main.h`
- **每一幀的工作**:
    1.  **輸入處理 (Input)**: 檢查鍵盤、滑鼠、手柄狀態。
    2.  **UI 更新**: 更新選單動畫與消息。
    3.  **物理更新 (Havok)**: 計算碰撞、重力與布娃娃系統。
    4.  **AI 更新 (Actor Update)**: 這是最耗時的部分，決定了 NPC 的動作。
    5.  **腳本更新 (VM Update)**: 執行 Papyrus 腳本隊列。
    6.  **渲染 (Renderer)**: 將計算好的場景提交給顯示卡。

---

## 2. NPC 的行為等級 (AI Update Levels)

引擎不會在每一幀都更新世界上所有的 NPC。為了優化性能，`RE::AIProcess` 將 NPC 分為四個等級：

- **原始碼**: `include/RE/A/AIProcess.h`

1.  **High (高等級)**: 玩家身邊的 NPC。每一幀都更新，擁有完整的 3D 動畫和精確的路徑規劃。
2.  **Middle-High**: 稍遠處的 NPC。更新頻率略低。
3.  **Middle-Low**: 遠處或被遮擋的 NPC。僅執行基本的邏輯判斷，動畫可能被簡化或停止。
4.  **Low (低等級)**: 遠在其他區域的 NPC。僅計算位置移動，不處理任何物理碰撞或 3D 動畫。

---

## 3. 離線 AI：看不到的 NPC 在做什麼？

當玩家不在某個區域時，該區域的單元（Cell）會被“卸載”（Unloaded）。

- **卸載狀態的 NPC**: 
    - 如果 NPC 不是“持久化（Persistent）”的，他們會暫時消失。
    - 如果是持久化 NPC（如重要領主、隊友），他們的 3D 模型會被銷毀，但其 **數據對象 (REFR)** 依然保留在內存中。
- **背景旅行 (Background Travel)**:
    - 當 NPC 處於卸載區域且有“旅行（Travel）”包裹時，引擎不會計算路徑。
    - **計算公式**: `當前位置 = 起點 + (流逝時間 * 移動速度)`。
    - 這就是為什麼你快讀（Fast Travel）後，會發現隊友過一會才追上來，或者 NPC 在你等待後出現在了目的地。

---

## 4. 地形與路徑導航 (Navmesh & Pathing)

NPC 如何在複雜地形旅行？

- **Navmesh (導航網格)**: 
    - **原始碼**: `include/RE/B/BSNavmesh.h`
    - 這是遊戲世界的“底圖”，標註了哪些地方可以走，哪些地方是障礙。
- **Pathing (路徑查找)**:
    - NPC 使用 A* 算法在 Navmesh 上尋找最短路徑。
    - **動態障礙**: 如果路徑上出現了玩家放置的建築（如你的龍吼生成的床），NPC 的 AI 會檢測到碰撞並嘗試繞路，這發生在 `RE::Pathing` 模塊中。

---

## 5. 事件如何觸發 (Event Dispatching)

Skyrim 的事件系統基於觀察者模式（Observer Pattern）。

- **原始碼**: `include/RE/B/BSTEvent.h`
- **觸發過程**:
    1.  **引擎動作**: 例如 `Actor::Die()` 函數被調用。
    2.  **事件發射**: 該函數內部會調用 `DeathEventSource::SendEvent()`。
    3.  **分發**: 遍歷所有已註冊的 `BSTEventSink`，並調用他們的 `ProcessEvent` 方法。
    4.  **順序**: 某些事件是同步的（立即執行），某些是異步的（推入下一幀隊列）。

---

## 6. 技術總結與開發啟發

- **性能優化**: 你的插件代碼如果要在每一幀運行，請務必檢查玩家是否在場（`Is3DLoaded()`）。不要在後台計算成千上萬個卸載 NPC 的複雜邏輯。
- **離線模擬**: 如果你要製作一個“NPC 模擬生活”模組，你應該操作 `TESPackage` 的數據，而不是嘗試操作 NPC 的坐標。
- **Hook 點**: 
    - `Main::Update`: 適合做全局系統更新（如天氣切換、時間控制）。
    - `Actor::Update`: 適合做單體行為控制。

---

## 7. 核心流程圖

```text
[ RE::Main Loop ]
      |
      +-- 物理 (Havok)
      |
      +-- AI 處理 (AIProcess)
      |     |-- [High] 完全模擬 (玩家視野內)
      |     |-- [Low] 位置插值 (玩家視野外/卸載區域)
      |
      +-- 事件分發 (BSTEventSource) ---> [ 你的 C++ 插件 ]
      |
      +-- 腳本渲染 (VM/Renderer)
```
