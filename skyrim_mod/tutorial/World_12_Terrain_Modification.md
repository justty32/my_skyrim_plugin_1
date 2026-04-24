# 12. 技術實戰：環境生成與地形調整

本教學教您如何在準星指向的地表生成自然景觀（山石、樹木），並討論動態地形修改的技術邊界。

## 1. 生成自然景觀 (山石與樹木)

在 C++ 中生成樹木或石頭與生成床鋪邏輯一致，但需要注意「碰撞」與「地面貼合」。

```cpp
void SpawnEnvironmentDecor() {
    auto player = RE::PlayerCharacter::GetSingleton();
    
    // 1. 獲取準星位置
    auto crosshairRef = SKSE::GetCrosshairRef();
    RE::NiPoint3 spawnPos;
    if (crosshairRef) {
        spawnPos = crosshairRef->GetPosition();
    } else {
        return;
    }

    // 2. 選擇對象 (PlaceHolder ID)
    // 0x00038432 - 某種松樹 (Static Tree)
    // 0x0001B983 - 某種大山石 (Static Rock)
    auto rockBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0001B983);
    
    if (rockBase) {
        // 3. 生成對象
        auto newRock = player->PlaceAtMe(rockBase, 1, false, false);
        if (newRock) {
            newRock->SetPosition(spawnPos);
            // 隨機旋轉讓景觀更自然
            float randomRot = static_cast<float>(rand() % 360);
            newRock->SetAngle(0.0f, 0.0f, randomRot);
        }
    }
}
```

## 2. 關於「調整地形」 (Terrain Sculpting)

**警告**：Skyrim 的地形（Heightmap）是由預生成的 `.btr` / `.bnd` 數據文件定義的，這與物體（Static Objects）不同。

### A. 在純插件中修改地形的困難點：
-   **靜態性**: 引擎在加載 Cell 時會將高度圖讀入物理內存，即時修改高度圖需要極深層次的內核 Hook。
-   **Navmesh 崩潰**: 修改地形後，導航網格（NPC 走路的路徑）不會更新，會導致 NPC 懸空或穿地。

### B. 常見的替代方案：
1.  **生成「地形補丁」**:
    如果你想讓地面隆起，不要去改高度圖，而是生成一個巨大的「地表紋理山石（Landscape Static）」。這就是大多數「築牆術」模組的做法。
2.  **使用 Decals (貼花)**:
    如果你只是想改變地面的視覺外觀（如燒焦、結冰），應使用 `RE::BGSDecalManager`。

## 3. 關鍵 API 標註
-   **`TESObjectREFR::PlaceAtMe`**: 生成實體。`include/RE/T/TESObjectREFR.h`
-   **`TESObjectTREE`**: 專門處理具備風吹動畫的樹木類別。`include/RE/T/TESObjectTREE.h`
-   **`RE::NiPoint3`**: 用於計算生成位置。`include/RE/N/NiPoint3.h`

## 4. 總結
在 Skyrim 中，「造山」和「種樹」其實都是在正確的位置 `PlaceAtMe` 生成對應的靜態模型（Statics）。除非你是製作引擎級的編輯器，否則請避免直接嘗試修改引擎的高度圖數據。
