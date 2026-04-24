# 07. 進階實戰：在準星位置生成房屋

本教學將展示如何生成大型靜態建築（Static Objects）。與生成床鋪不同，房屋通常由多個部分組成，或是一個巨大的靜態模型，且需要精確處理生成位置以防止「入土」。

## 1. 核心邏輯
1.  攔截特定龍吼施放事件。
2.  利用 `Raycast` (射線檢測) 獲取地形精確坐標，或直接獲取準星指向位置。
3.  計算偏移量（Offset），確保房屋底部與地面齊平。
4.  調用 `PlaceAtMe` 生成房屋。

## 2. 代碼實現

```cpp
void SpawnHouseAtCrosshair() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    // 1. 獲取準星指向的物理坐標
    // 在純 C++ 中，我們通常通過玩家的視線射線來計算
    RE::NiPoint3 spawnPos;
    auto crosshairRef = SKSE::GetCrosshairRef();
    
    if (crosshairRef) {
        spawnPos = crosshairRef->GetPosition();
    } else {
        // 如果沒指著對象，則取玩家前方 500 單位的坐標
        // 這裡需要 NiPoint3 的向量運算
        spawnPos = player->GetPosition();
        // 簡化示意：
        spawnPos.x += 500.0f; 
    }

    // 2. 生成房屋 (PlaceHolder ID: 0x房屋靜態對象ID)
    auto houseBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0xABC123);
    
    if (houseBase) {
        // 3. 修正 Z 軸，防止房屋埋在地下
        // 房屋模型的重心可能在中心，需要向上移動一段距離
        spawnPos.z += 100.0f; 

        // 4. 生成並設置旋轉（讓房屋面向玩家）
        auto newHouse = player->PlaceAtMe(houseBase, 1, false, false);
        if (newHouse) {
            newHouse->SetPosition(spawnPos);
            // 讓房屋的旋轉角度與玩家一致
            newHouse->SetAngle(0.0f, 0.0f, player->GetAngleZ());
            
            RE::DebugNotification("房屋已拔地而起！");
        }
    }
}
```

## 3. 關鍵 API 標註
-   **`SetPosition()` / `SetAngle()`**: 手動調整生成後的實體位置與旋轉。`include/RE/T/TESObjectREFR.h`
-   **`RE::NiPoint3`**: 引擎使用的三維向量類。`include/RE/N/NiPoint3.h`

## 4. 技巧提示
-   **靜態對象 (Static)**: 房屋通常是 `Static` 類型，生成後不可移動。
-   **室內導航 (Navmesh)**: 動態生成的房屋不會自動生成 Navmesh，NPC 可能會穿牆或無法進入，這需要更高級的動態導航技術。
-   **PlaceHolder**: 對於房屋，你可以嘗試使用 `0x00012E47` (這是一個雪漫城的小屋模型)。
