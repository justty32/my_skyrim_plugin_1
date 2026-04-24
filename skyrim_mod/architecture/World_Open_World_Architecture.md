# Skyrim 開放世界架構：動態網格加載與單元管理 (Technical Deep Dive)

Skyrim 的開放世界並非一次性加載，而是基於一套精密、異步的網格系統（Grid System）。理解這一點對於開發涉及遠距離物體操作、動態生成建築或優化性能的模組至關重要。

---

## 1. 核心概念：網格 (The Grid)

在室外（Exteriors）中，世界被劃分為無數個座標為 $(X, Y)$ 的 **單元 (Cell)**。

- **uGridsToLoad (預設值: 5)**: 
    - 這是控制引擎加載範圍的核心參數。
    - **5x5 範圍**: 引擎會以玩家所在單元為中心，加載周圍 $5 \times 5$ 的單元（共 25 個）。
    - **Active Cells**: 只有這 25 個單元內的物體（NPC, 物理對象）是處於激活（Active）狀態並參與主循環更新的。
    - **加載邊界**: 當玩家跨越單元邊界（Cell Boundary）時，引擎會卸載後方的一排 5 個單元，並加載前方的新一排 5 個單元。

---

## 2. 異步加載流程 (I/O & Persistence)

當玩家移動時，加載過程是異步發生的，以避免畫面卡頓。

- **原始碼**: `RE::TESDataHandler` 與 `RE::TESFile` 負責從 `.esp/.bsa` 讀取數據。
- **加載順序**:
    1.  **Landscape (地形)**: 首先加載高度圖與貼圖。
    2.  **Statics (靜態物)**: 房屋、樹木、岩石。
    3.  **References (引用)**: NPC、門、容器、拾取物。
    4.  **Navmesh (導航網格)**: 為了讓 AI 能立即在加載區域移動。
- **IO 線程**: 引擎使用背景線程預讀數據。如果你快速移動（如使用 `SetPos`），可能會看到「空洞的地形」或「浮空的物件」，這就是 IO 線程趕不上渲染線程的現象。

---

## 3. 座標轉換 (Coordinate Transformation)

Skyrim 使用兩套座標系統：

- **單元座標 (Cell Coordinates)**: 如 Tamriel $(0, 0)$ 代表白漫城（Whiterun）附近。
- **世界座標 (World Coordinates)**: 遊戲內實際的 $X, Y, Z$ 浮點數值。
- **轉換公式**:
    - 單元邊長 = 4096 遊戲單位 (Units)。
    - $CellX = \lfloor WorldX / 4096 \rfloor$
    - $CellY = \lfloor WorldY / 4096 \rfloor$

在 C++ 中獲取當前單元：
```cpp
auto player = RE::PlayerCharacter::GetSingleton();
auto currentCell = player->GetParentCell();
if (currentCell && !currentCell->IsInterior()) {
    auto grid = currentCell->GetCoordinates();
    // grid->cellX, grid->cellY
}
```

---

## 4. 遠景系統 (LOD - Level of Detail)

為了讓玩家看到超出 $5 \times 5$ 範圍的景觀，引擎使用了 LOD。

- **Object LOD**: 超出加載範圍的建築會被替換為極低面數的靜態模型（`*.bto` 檔案）。這些物體**不具備**腳本功能和碰撞。
- **Terrain LOD**: 遠方的山脈是簡化的高度圖（`*.btr` 檔案）。
- **Distant Tree**: 遠方的樹木通常只是兩個平面交叉的十字面片（Quads）。

---

## 5. 持久化對象 (Persistent Objects)

並非所有東西在 $5 \times 5$ 範圍外都會被卸載。

- **Persistent Cell**: 每個 WorldSpace 都有一個虛擬的「持久化單元」。
- **內容**: 任務目標、隊友、全局腳本控制的物體。
- **技術細節**: 即使在千里之外，這些對象的 `RE::TESObjectREFR` 指針在內存中依然有效，但其 3D 模型（`NiNode`）會被銷毀。

---

## 6. 技術開發啟發

1.  **跨單元生成**: 如果你在玩家移動時動態生成物體，請務必計算其是否在 $5 \times 5$ 加載範圍內。如果在範圍外生成，模型不會立即出現（直到玩家靠近）。
2.  **性能陷阱**: 頻繁在 `Update` 循環中調用 `GetParentCell()` 或遍歷加載單元內的物體是昂貴的。應使用 `RE::TESObjectREFR::GetHandle()` 或事件監聽（如 `CellFullyLoaded`）。
3.  **邊界檢測**: 檢測玩家是否跨越 Cell 是觸發「區域變更邏輯」（如切換背景音樂、重置生成器）的最佳時機。

---

## 7. 核心類別原始碼標註

- **`RE::TESWorldSpace`**: `include/RE/T/TESWorldSpace.h` - 管理整個網格系統。
- **`RE::GridArray`**: `include/RE/G/GridArray.h` - 實際管理內存中加載單元的陣列（通常是 5x5）。
- **`RE::TESObjectCELL`**: `include/RE/T/TESObjectCELL.h` - 單個網格容器。
- **`RE::BGSReferenceLoadEvent`**: 監聽物體加載完成的事件。
