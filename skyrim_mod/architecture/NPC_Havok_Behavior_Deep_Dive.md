# Skyrim 動作邏輯核心：Havok Behavior (.hkx) 深度解析

如果 NIF 是角色的「肉體」，那麼 `.hkx` 文件就是角色的「神經反射系統」。它定義了角色在不同狀態下如何切換動作、如何混合動畫以及如何響應外界信號。

---

## 1. .hkx 文件的本質

Skyrim 使用 **Havok SDK** 來處理動畫邏輯。`.hkx` 是一種二進制序列化格式，內部儲存了 Havok 類別的實例。

### 行為圖 (Behavior Graph) 的層級：
1.  **`Character.hkx`**: 定義角色的骨骼、可用的行為圖文件路徑以及物理屬性。
2.  **`Behavior.hkx`**: **核心邏輯圖**。包含狀態機（State Machines）和混合樹（Blend Trees）。
3.  **`Animation.hkx`**: 實際的動作捕捉數據（頂點位移、骨骼旋轉）。

---

## 2. 行為圖的運作流程

行為圖就像一個複雜的「流量控制中心」，其運作遵循以下鏈條：

### A. 輸入：變量 (Variables) 與事件 (Events)
- **變量**: 如 `Speed`, `IsSneaking`。C++ 插件修改這些值，圖表會根據數值跳轉到不同分支。
- **事件**: 如 `JumpUp`。這是一個「信號」，觸發圖表立即執行某個動作。

### B. 處理：狀態機 (State Machines)
- 引擎會判斷當前處於哪個狀態（如：`Idle` 狀態）。
- 如果 `Speed > 0`，狀態機會順著連線過渡到 `Walk` 狀態。

### C. 輸出：混合數據 (Blending)
- 最終，圖表會輸出每一根骨骼的旋轉數據。
- **混合範例**: 如果玩家正在「一邊走路一邊舉火把」，圖表會將「下半身走路」和「上半身舉火把」兩個動畫進行數學上的加權平均。

---

## 3. 資源管理機制

`.hkx` 資源由引擎的動畫系統與 `BSResource` 協同管理：

1.  **緩存共享 (Static Graph)**: 
    - 所有的「強盜」或「衛兵」通常共用同一個 `HumanoidBehavior.hkx`。
    - 引擎只會在內存中加載一份圖表邏輯（唯讀），以節省空間。
2.  **實例數據 (Runtime Instance)**:
    - 雖然圖表邏輯是共享的，但每個 NPC 擁有一份獨立的「變量副本」（如：強盜 A 正在跑，強盜 B 正在坐）。
3.  **加載時機**: 
    - 當 Actor 首次被加載到 3D 世界時，`RE::BSAnimationGraphManager` 會請求對應的 `.hkx`。
    - 引擎會透過 `Data\Meshes\Actors\...\Behaviors\` 尋找文件。

---

## 4. C++ 插件中的深度控制

透過 CommonLibSSE-NG，你可以直接與 Havok 對象交互：

```cpp
void InspectBehavior(RE::Actor* a_actor) {
    auto manager = a_actor->GetAnimationGraphManager();
    if (manager) {
        // 獲取 Havok 內部的行為圖實例
        for (auto& graph : manager->graphs) {
            auto hkbGraph = graph->behaviorGraph;
            // 你可以在這裡遍歷 Havok 的節點樹 (hkbNode)
        }
    }
}
```

---

## 5. 核心類別原始碼標註

- **`RE::hkbBehaviorGraph`**: `include/RE/H/hkbBehaviorGraph.h` - 行為圖主體。
- **`RE::hkbStateMachine`**: `include/RE/H/hkbStateMachine.h` - 狀態機邏輯。
- **`RE::hkbVariableValue`**: `include/RE/H/hkbVariableValue.h` - 行為圖變量。
- **`RE::BSAnimationGraphManager`**: `include/RE/B/BSAnimationGraphManager.h` - Skyrim 對 Havok 的包裝器。

---

## 6. 技術總結
- **`.hkx` 不是動畫片**，它是**動畫的說明書**。
- 修改動畫表現有兩條路：
    1.  更換 `.hkx` 引用的動畫資源（換皮）。
    2.  透過 C++ 修改圖表變量（控制邏輯）。
- **資源管理**: 採取「邏輯共享，變量獨立」的策略。
 village
