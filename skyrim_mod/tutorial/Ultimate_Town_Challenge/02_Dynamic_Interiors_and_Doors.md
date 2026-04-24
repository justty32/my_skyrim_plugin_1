# 終極挑戰：讓每一棟房屋都能進入 (動態傳送門技術)

在上一篇中，我們在野外生成了房屋的外殼（Exterior）。但這只是一個空殼實體，玩家無法進入。在 Skyrim 引擎中，「進入房屋」本質上是玩家與一扇門互動，觸發了**加載畫面（Loading Screen）**，並被傳送到另一個名為「室內單元（Interior Cell）」的封閉空間。

要完全透過 C++ 實現這一點，我們必須動態生成門，並利用 `ExtraTeleport` 數據將它們鏈接起來。

## 1. 核心概念：Teleport Door (傳送門)

在引擎中，一扇可以進入另一個空間的門需要具備以下條件：
1.  門的實體是一個 `RE::TESObjectREFR`（引用），其基礎表單類型必須是 `Door`。
2.  該引用的 `ExtraDataList` 中必須包含 `RE::ExtraTeleport`。
3.  `RE::ExtraTeleport` 包含了 `RE::DoorTeleportData`，其中儲存了**目標門的 Handle (引用句柄)**、目標位置座標與進入後的旋轉角度。

## 2. 室內空間從哪裡來？ (Interior Cell)

**技術瓶頸**：純 C++ 插件*幾乎不可能*憑空創造一個完整的室內 `TESObjectCELL` 並為其動態加載牆壁、地板與燈光。
**解決方案**：我們必須依賴 **「空白佔位室內 (Dummy Interior Cells)」**。

你需要在編寫插件時，準備一個極簡的 ESP（包含 100 個完全一樣的空房間，但不要在遊戲世界中放置任何門連通它們）。當你在 C++ 中生成城鎮時，動態地將野外的門鏈接到這些未被使用的空房間內。

## 3. 代碼實現：動態鏈接兩扇門

假設我們已經生成了一扇外部的門（`extDoor`）和一扇室內的門（`intDoor`）。

```cpp
#include <RE/Skyrim.h>

// 該函數用於將門 A 鏈接到門 B
void LinkDoors(RE::TESObjectREFR* doorA, RE::TESObjectREFR* doorB) {
    if (!doorA || !doorB) return;

    // 獲取或創建門 A 的額外數據列表
    if (!doorA->extraList.HasType(RE::ExtraDataType::kTeleport)) {
        // Skyrim 中 Teleport 數據極其複雜，它通常依賴底層分配
        // 這是高度簡化的概念代碼
        
        auto teleportExtraA = new RE::ExtraTeleport();
        auto teleportDataA = new RE::DoorTeleportData();

        // 設置門 A 的目標為門 B
        // (注意：需要使用 Handle 而非原始指針)
        teleportDataA->linkedDoor = doorB->CreateRefHandle(); 
        
        // 設置玩家穿過門 A 後，出現在門 B 前方的位置
        // (假設門 B 朝向 Y 軸正方向，我們將玩家放在門前一點點)
        auto doorBPos = doorB->GetPosition();
        doorBPos.y += 100.0f; // 向前偏移
        teleportDataA->position = doorBPos;
        
        // 設置玩家出現後的面朝方向 (通常與門 B 背對)
        auto doorBRot = doorB->GetAngle();
        doorBRot.z += 3.14159f; // 旋轉 180 度 (弧度制)
        teleportDataA->rotation = doorBRot;

        teleportExtraA->teleportData = teleportDataA;
        doorA->extraList.Add(teleportExtraA);
    }

    // 雙向鏈接：讓門 B 也能傳送回門 A
    if (!doorB->extraList.HasType(RE::ExtraDataType::kTeleport)) {
        auto teleportExtraB = new RE::ExtraTeleport();
        auto teleportDataB = new RE::DoorTeleportData();

        teleportDataB->linkedDoor = doorA->CreateRefHandle();
        
        auto doorAPos = doorA->GetPosition();
        doorAPos.y += 100.0f; 
        teleportDataB->position = doorAPos;
        
        auto doorARot = doorA->GetAngle();
        doorARot.z += 3.14159f;
        teleportDataB->rotation = doorARot;

        teleportExtraB->teleportData = teleportDataB;
        doorB->extraList.Add(teleportExtraB);
    }
    
    RE::DebugNotification("時空鏈接完成，房屋現在可以進入了！");
}
```

## 4. 關鍵 API 標註
-   **`RE::ExtraTeleport`**: 儲存傳送數據的容器。`include/RE/E/ExtraTeleport.h`
-   **`RE::DoorTeleportData`**: 具體的傳送目標實體與座標。`include/RE/D/DoorTeleportData.h`
-   **`CreateRefHandle()`**: 為 `TESObjectREFR` 創建安全引用句柄，用於跨單元鏈接。

## 5. 總結與自動化思路

要讓生成的每一個房屋都能進入，你的插件需要一套強大的**分配器 (Allocator) 系統**：

1.  讀取 ESP 中預留的 100 個空白室內單元 ID。
2.  每次在野外生成一棟房屋，就從池子裡提取一個未使用的室內單元。
3.  在野外房屋模型前 `PlaceAtMe` 一扇外部門。
4.  在室內單元的固定位置 `PlaceAtMe` 一扇內部門。
5.  調用 `LinkDoors` 將它們雙向綁定。
6.  在室內單元中動態生成床鋪、火爐與家具（參考教程 04）。

---

## 6. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/E/ExtraTeleport.h` - 傳送數據容器。
- `include/RE/D/DoorTeleportData.h` - 目錄詳情。
- `include/RE/T/TESObjectDOOR.h` - 門的定義。
- `include/RE/B/BSPointerHandle.h` - 引用句柄管理。

### 推薦使用的函數
- `RE::TESObjectREFR::CreateRefHandle()`：創建安全的門句柄。
- `RE::ExtraDataList::Add(ExtraData)`：為門附加傳送組件。
- `RE::TESObjectREFR::GetPosition()`：獲取門的物理座標以計算傳送落點。
- `RE::TESObjectREFR::Activate(Activator, ...)`：透過插件腳本觸發開門動作。
