# 32. 技術實戰：萬物皆可飛 (物體巡航飛行系統)

本教學展示如何對世界中的實體（Reference）施加動態軌跡。我們將實現：對著一個木桶或一把劍施法，它會立刻騰空而起，並沿著一個預設的圓形或來回軌跡持續巡航。

## 1. 核心邏輯
1.  **鎖定目標**: 透過施法或準星獲取目標 `RE::TESObjectREFR`。
2.  **禁用重力**: 將物體切換到「運動學 (Kinematic)」狀態，使其不受原版重力干擾。
3.  **軌跡規劃**: 定義一組座標點（Waypoints）。
4.  **循環平滑移動**: 利用 `TranslateTo` 實現點到點的平滑飛行，並在到達終點後重新開始。

## 2. 代碼實現：巡航引擎

```cpp
#include <RE/Skyrim.h>

void StartObjectCruise(RE::TESObjectREFR* a_target) {
    if (!a_target) return;

    // 1. 獲取當前座標
    auto startPos = a_target->GetPosition();
    
    // 2. 設置巡航路徑點 (以起點為中心的四個點)
    std::vector<RE::NiPoint3> waypoints = {
        { startPos.x + 500, startPos.y, startPos.z + 200 },
        { startPos.x, startPos.y + 500, startPos.z + 200 },
        { startPos.x - 500, startPos.y, startPos.z + 200 },
        { startPos.x, startPos.y - 500, startPos.z + 200 }
    };

    // 3. 核心：啟動 TranslateTo 平滑移動
    // 注意：TranslateTo 是異步的，引擎會自動處理每一幀的位移
    // 參數：目標位置, 目標旋轉, 線速度, 旋轉速度
    RE::NiPoint3 currentRot = a_target->GetAngle();
    a_target->TranslateTo(waypoints[0], currentRot, 300.0f, 0.0f);

    // 4. 循環邏輯 (進階)
    // 你需要監聽 RE::TESMoveFinishEvent (移動完成事件)
    // 當物體到達 waypoints[0] 時，在事件回調中再次調用 TranslateTo 指向 waypoints[1]
    
    RE::DebugNotification("巡航模式啟動：物體已進入軌道。");
}
```

## 3. 技術細節解析 (3D 與物理)

### A. 為什麼物體不會掉下來？
調用 `TranslateTo` 會暫時接管物體的物理狀態。引擎會將其從 Havok 的重力模擬中移除，轉而由「運動學控制器」驅動。這就是為什麼你可以讓一個沈重的鐵製保險箱像氣球一樣在天上飛。

### B. 旋轉的處理
如果你希望物體在飛行時不斷旋轉（例如旋轉的劍）：
- 修改 `TranslateTo` 的最後一個參數（旋轉速度）。
- 或者每一幀手動增加 `a_target->GetAngle().z`。

### C. 碰撞處理
即便物體在巡航，它依然具備物理碰撞。如果你在它飛行的路徑上擋住它，NPC 會被撞開。

## 4. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/T/TESObjectREFR.h` - 位移核心。
- `include/RE/N/NiPoint3.h` - 空間座標。

### 推薦使用的函數
- `RE::TESObjectREFR::TranslateTo(...)`：讓物體像載具一樣移動的最穩定的 API。
- `RE::TESObjectREFR::StopTranslation()`：立刻停止飛行並讓物體摔回地面。
- `RE::TESObjectREFR::IsSearching()`：檢查物體是否正在進行路徑位移。

## 5. 擴展思路
-   **浮空術**: 將 `nextPos` 設置在玩家頭頂上方，物體就會一直跟著玩家。
-   **幽靈武器**: 對著掉在地上的劍施法，讓它在戰場上盤旋並攻擊附近的敵人（結合教學 22 的 AI 邏輯）。
