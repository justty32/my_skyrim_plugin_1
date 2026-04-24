# Skyrim 引擎核心架構：從數據到世界的演變

要開發 SKSE 插件，必須理解 Bethesda 引擎的底層哲學：**“數據（Data）與實例（Instance）的徹底分離”**。

---

## 1. 核心層級結構 (The Hierarchy)

Skyrim 引擎的對象鏈條可以簡化為：
**`TESForm` (數據藍圖) -> `TESBoundObject` (物理模型藍圖) -> `TESObjectREFR` (世界實體) -> `Actor` (活著的實體)**

### A. TESForm：數據的根源
- **原始碼**: `include/RE/T/TESForm.h`
所有的東西（法術、天氣、種族、武器定義）都是 `TESForm`。
- 它存儲在 `.esm` 或 `.esp` 文件中。
- 它不具備位置座標。
- **比淵**: 它是“蘋果”的定義——知道蘋果是紅色的，能吃，值多少錢。

### B. TESBoundObject：具備形體的數據
- **原始碼**: `include/RE/T/TESBoundObject.h`
繼承自 `TESForm`。代表有體積、有 3D 模型的靜態數據。
- 例如：`TESObjectWEAP` (武器, `include/RE/T/TESObjectWEAP.h`), `TESObjectARMO` (護甲, `include/RE/T/TESObjectARMO.h`)。
- **比淵**: 它是“蘋果的模型”——知道蘋果在 3D 空間中長什麼樣，邊界框（Bound）有多大。

### C. TESObjectREFR：實體引用 (Ref)
- **原始碼**: `include/RE/T/TESObjectREFR.h`
這是引擎最核心的動態類。它將 `TESBoundObject` 放置到遊戲世界中。
- 它包含 **座標 (Location)**、**旋轉 (Rotation)** 和 **縮放 (Scale)**。
- 它包含 **ExtraDataList** (`include/RE/E/ExtraDataList.h`)：存儲這個特定蘋果的動態狀態（例如：它是否被玩家偷過？它是否被附魔了？）。
- **比淵**: 它是“桌子上那個特定的蘋果”。

---

## 2. 玩家與角色的特殊性 (Actor & Player)

當一個 `Reference` 需要具備 AI、屬性（生命值）和動作時，它就演變成了 `Actor`。

### Actor (角色)
- **原始碼**: `include/RE/A/Actor.h`
- 繼承自 `TESObjectREFR`。
- **Base Data**: 指向一個 `TESNPC` (`include/RE/T/TESNPC.h`)（定義了角色的臉、初始屬性、種族）。
- **Instance Data**: 包含當前生命值、當前裝備、正在執行的 AI Package。

### PlayerCharacter (玩家)
- **原始碼**: `include/RE/P/PlayerCharacter.h`
- 繼承自 `Character` (`include/RE/C/Character.h`) -> `Actor`。
- 它是全局唯一的單例。
- **特殊之處**: 它擁有玩家專屬的輸入處理、任務日誌和技能經驗系統。

---

## 3. 引擎如何將一切鏈接起來？

### 第一步：加載數據 (The Definition)
當遊戲啟動時，`TESDataHandler` 讀取所有插件，將所有的 `TESForm` 加載到內存。此時，世界還不存在。

### 第二步：加載單元 (The Cell)
當玩家進入某個區域（如雪漫城）：
1. 引擎加載 `TESObjectCELL`（單元）。
2. 單元包含一個 `REFR` 列表（該單元內所有的椅子、桌子、NPC）。
3. 引擎遍歷這些 `REFR`，根據它們指向的 `TESBoundObject`（模型數據）去磁盤加載 3D 模型（NIF 文件）。

### 第三步：更新循環 (The Main Loop)
在每一幀中：
1. **AI 系統**: 更新所有 `Actor` 的 AI 邏輯。
2. **物理系統 (Havok)**: 更新 `REFR` 的碰撞和位置。
3. **渲染系統**: 根據 `REFR` 的位置和模型數據，將畫面繪製到屏幕上。

---

## 4. 關鍵組件的交互模式

### 1. 數據與實例的查找
如果你想修改玩家手裡的劍：
- `PlayerCharacter` -> `Inventory` (物品欄) -> `InventoryEntryData`。
- 這個數據會指向一個 `TESObjectWEAP` (靜態數據，定義傷害)。
- 但它也會包含 `ExtraData` (動態數據，定義這個特定的劍是否有自定義名稱或附魔)。

### 2. 事件驅動 (Events)
引擎通過 `BSTEventSource` 發送消息。例如：
- 玩家按下按鍵 -> `InputEvent`。
- 準星對準物體 -> `CrosshairRefEvent`。
- 這些事件通常會傳遞一個 `TESObjectREFR*` 給你，讓你進行後續操作。

---

## 5. 開發總結：我該操作什麼？

- 如果你要修改**遊戲機制**（例如：改變所有鐵劍的重量）：修改 `TESObjectWEAP` (Base Data)。
- 如果你要修改**特定對象**（例如：讓玩家指著的那個蘋果爆炸）：操作 `TESObjectREFR` (Reference)。
- 如果你要修改**生物行為**（例如：讓某個 NPC 逃跑）：操作 `Actor`。
- 如果你要獲取**全局信息**（例如：玩家當前坐標）：訪問 `PlayerCharacter` 單例。

---

## 6. 核心架構圖

```text
[ .esp / .esm Data ]
      |
      v
  TESForm (Base Data: Price, Name, Type)
      |
      +---> TESBoundObject (Add Model, Bounds)
                |
[ Game World Runtime ]
                |
                v
          TESObjectREFR (Add Position, Rotation, ExtraData)
                |
                +---> Actor (Add AI, Stats, Inventory)
                          |
                          +---> PlayerCharacter (Add Skills, Quests, Input)
```

理解了這個流程，你就明白了為什麼在代碼中，我們總是先從一個 `FormID` 找到 `TESForm`，然後將其轉化為 `Reference`，最後再進行操作。
