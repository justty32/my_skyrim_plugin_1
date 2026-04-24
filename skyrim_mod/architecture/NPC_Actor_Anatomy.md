# Skyrim Actor 解剖學：角色的組成構造

要深入操控 NPC 或玩家，必須理解 `RE::Actor` 類別在引擎中是如何被“拼湊”出來的。一個 Actor 並非單一對象，而是多個複雜系統的聚合體。

---

## 1. 基礎模板：`TESNPC` (The Blueprint)
在 `Actor` 實體生成之前，所有的核心屬性都定義在它的模板中。
- **原始碼**: `include/RE/T/TESNPC.h`
- **組成內容**: 
    - **Race (種族)**: 決定了骨骼、基礎抗性、可用動作。
    - **Class (職業)**: 決定了屬性的成長權重。
    - **FaceGen Data**: 儲存了捏臉數據。

---

## 2. 物理外觀 (Visuals & Morph)

Actor 的視覺表現由 3D 模型與材質層疊而成。

### A. Race (種族)
- **原始碼**: `include/RE/T/TESRace.h`
- 它是外觀的根源。種族定義了 Actor 使用哪套 `.nif` 模型（Body Mesh）。

### B. 體型與比例
- **Height (身高)** & **Weight (體重)**: 
    - `weight` 影響頂點變形（Morphing），即“肌肉/肥胖”程度。
    - 透過 `actor->GetWeight()` 獲取。

### C. 裝備外觀 (Worn Armor)
- 當 Actor 穿上裝備時，引擎會隱藏對應部位的皮膚 Mesh，並加載裝備的 Mesh。

---

## 3. 能力與屬性 (Actor Values)

Actor 的所有能力數值都透過 **Actor Value (AV)** 系統管理。

- **原始碼**: `include/RE/A/ActorValues.h`
- **數值組成**:
    - **Base Value**: 模板定義的基礎值。
    - **Permanent Modifiers**: 永久加成（如天賦、裝備）。
    - **Temporary Modifiers**: 臨時加成（如藥水、法術效果）。
    - **Current Value**: 扣除傷害後的當前值（如當前 HP）。

---

## 4. 魔法狀態 (Magic Target)

`Actor` 繼承自 `MagicTarget`，使其能夠成為法術的載體。

- **原始碼**: `include/RE/M/MagicTarget.h`
- **ActiveEffect 列表**: 存儲了當前所有作用在身上的效果（抗性、燃燒、隱身）。

---

## 5. 物品欄與所有權 (Inventory)

Actor 透過 `InventoryChanges` 管理隨身攜帶的物品。

- **原始碼**: `include/RE/I/InventoryChanges.h`
- **組成**:
    - **Base Inventory**: 模板中定義的初始物品。
    - **Dynamic Changes**: 遊戲中獲得、丟棄或裝備的記錄。

---

## 6. 行為大腦 (AI Process)

這是賦予 Actor “靈魂”的部分。

- **原始碼**: `include/RE/A/AIProcess.h`
- **組成**:
    - **MiddleHighProcess**: 處理當前正在裝備的武器、施放的法術狀態。
    - **Combat-Data**: 當前對玩家的仇恨值、戰鬥目標。

---

## 7. 總結：Actor 的組成清單

如果你在 C++ 中要“造”一個 Actor，你需要準備：
1.  **`TESRace*`**: 定義物種。
2.  **`TESNPC*` (Base)**: 定義身份與初始屬性。
3.  **`ActorValueOwner`**: 管理 HP/MP/技能。
4.  **`InventoryChanges`**: 管理背包。
5.  **`ActorState`**: 管理姿勢（潛行、游泳）。

## 8. 核心類別原始碼標註

- **`RE::Actor`**: `include/RE/A/Actor.h` - 總入口。
- **`RE::TESRace`**: `include/RE/T/TESRace.h` - 生理基礎。
- **`RE::ActorValue`**: `include/RE/A/ActorValues.h` - 數值定義。
- **`RE::AIProcess`**: `include/RE/A/AIProcess.h` - 行為決策。
