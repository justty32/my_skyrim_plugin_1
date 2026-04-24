# 04. 終極挑戰：程序化室內佈局 (Procedural Interior Generation)

上一篇解決了室外城的拼接，現在我們要深入「房屋內部」。透過將室內空間模塊化（地板模塊、牆壁模塊、樓梯模塊），我們可以讓同一個空殼房子，每次進去都有不同的格局。

## 1. 核心邏輯：在空白單元內「蓋房子」

1.  **準備工作**: 在 ESP 中建立一個巨大的、空無一物的 `TESObjectCELL` (Interior)。
2.  **定位系統**: 以該單元的 `(0, 0, 0)` 為基準坐標。
3.  **拼接算法**: 
    -   先鋪設地板（Floor Tiles）。
    -   檢測地板邊緣，拼接牆壁（Wall Tiles）。
    -   在牆壁接點處嘗試拼接門或窗戶。

## 2. 代碼實現：室內拼接範例

```cpp
void GenerateRandomRoom(RE::TESObjectCELL* a_targetCell) {
    auto player = RE::PlayerCharacter::GetSingleton();
    
    // 定義起始點
    RE::NiPoint3 currentCursor = { 0.0f, 0.0f, 0.0f };
    
    // 獲取地板模塊 (300x300 單位)
    auto floorBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0xINT_FLOOR_ID);

    // 鋪設 3x3 的地板
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            RE::NiPoint3 tilePos = { x * 300.0f, y * 300.0f, 0.0f };
            
            // 使用 PlaceAtMe 並指定目標 Cell
            // 注意：這是進階用法，通常需要先將玩家傳送到該單元
            auto tile = player->PlaceAtMe(floorBase, 1, true, false);
            tile->SetPosition(tilePos);
        }
    }
    
    // 添加光源 (否則室內是漆黑的)
    auto lightBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x000163BE); // 蠟燭光源
    player->PlaceAtMe(lightBase, 1, true, false)->SetPosition({150, 150, 100});
}
```

## 3. 解決室內導航與拼接的關鍵

### A. 室內 Navmesh 模塊
室內模塊的 Navmesh 必須非常嚴謹。如果你的地板模塊自帶了 300x300 的 Navmesh，拼在一起後，NPC 就能在房間內自由行走。

### B. 遮擋與剔除 (Room Bounds)
在大型室內空間中，為了性能，引擎使用 `Room Bounds` 和 `Portals`。
-   **純插件建議**: 對於動態生成的簡單室內，不要使用 Room Bounds，否則會出現家具消失的視覺 Bug。保持單元大小適中即可。

### C. 門的自動綁定
配合教學 02，在室內拼接完成後，找到其中一面「牆壁門模塊」，在該位置生成 `RE::ExtraTeleport` 門，將其連向野外的房屋外殼。

## 4. 終極目標：無限隨機地牢
這種技術的終極應用不是城鎮，而是**無限隨機地牢**。
1. 玩家進入一扇門。
2. C++ 插件從模塊庫中隨機選取 20 個模塊。
3. 自動拼接成迷宮。
4. 隨機撒上寶箱與 NPC（教學 18）。
---

## 5. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/T/TESObjectCELL.h` - 單元管理。
- `include/RE/P/PlayerCharacter.h` - 用於 PlaceAtMe 調用的主體。
- `include/RE/T/TESBoundObject.h` - 模塊模板。

### 推薦使用的函數
- `RE::PlayerCharacter::GetSingleton()->PlaceAtMe(...)`：在指定單元內生成對象。
- `RE::TESObjectREFR::SetPosition(NiPoint3)`：在室內單元的相對座標系中放置物體。
- `RE::TESObjectREFR::GetParentCell()`：獲取當前所在的室內單元指針。
- `RE::TESObjectCELL::IsInterior()`：驗證是否成功加載進室內空間。
