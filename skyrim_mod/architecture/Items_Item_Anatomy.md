# Skyrim 物品解剖學：物件的構造與屬性系統

Skyrim 的物品（Items）是一個由多個組件複合而成的複雜對象。理解物品的解剖結構，能讓你精確地修改武器傷害、藥水效果或添加自定義的觸發邏輯。

---

## 1. 基礎類型：`TESBoundObject` (Blueprint)
所有的物理物品都繼承自 `TESBoundObject`。
- **原始碼**: `include/RE/T/TESBoundObject.h`
- **關鍵功能**: 它定義了物體的「邊界（Bound）」，即它在 3D 空間中占據的大小。

---

## 2. 物理表現 (Model & Textures)

物品的外觀由模型與材質定義。

### A. 3D 模型 (Model)
- **原始碼**: `include/RE/T/TESModel.h` (作為基類包含)
- 定義了物品在世界中掉落時使用的 `.nif` 文件路徑。

### B. 第一人稱模型 (1st Person Model)
- 對於武器，還包含 `RE::TESModelTextureSwap`，定義了玩家拿在手裡時看到的精細模型。

---

## 3. 核心屬性 (Stats)

不同種類的物品有不同的屬性結構：

### A. 武器 (TESObjectWEAP)
- **原始碼**: `include/RE/T/TESObjectWEAP.h`
- **包含**: 基礎傷害（`attackDamage`）、攻擊速度（`speed`）、武器範圍（`reach`）、武器類型（單手、雙手、弓）。

### B. 護甲 (TESObjectARMO)
- **原始碼**: `include/RE/T/TESObjectARMO.h`
- **包含**: 護甲值（`armorRating`）、裝備部位（`slot`）。

### C. 藥水與消耗品 (AlchemyItem)
- **原始碼**: `include/RE/A/AlchemyItem.h`
- **包含**: 重量、價值、是否為自動計算價格。

---

## 4. 效果與附魔 (Effects & Enchantment)

物品的功能核心通常存儲在效果列表中。

- **Enchantable Object**: 武器和護甲可以攜帶 `RE::EnchantmentItem`。
- **Magic Effects**: 藥水則直接包含一個 `RE::Effect` 列表。
    - **原始碼**: `include/RE/E/Effect.h`
    - 每個效果都指向一個 `RE::EffectSetting` (Base Effect)，定義了具體功能（如：恢復生命、火傷）。

---

## 5. 事件觸發點 (Event Hooks)

物品與世界的交互通過特定的入口函數實現：

- **`OnActivate`**: 玩家點擊（撿起）物品時觸發。
- **`OnEquip` / `OnUnequip`**: 當 Actor 穿上或脫下該物品時觸發。
- **`OnHit`**: 武器擊中目標時觸發。
- **`OnContainerChanged`**: 物品被移動（存入箱子或丟棄）時觸發。

---

## 6. 動態實例數據：`ExtraDataList`

這是區分“鐵劍”與“你的精煉+10火傷鐵劍”的關鍵。

- **原始碼**: `include/RE/E/ExtraDataList.h`
- **動態屬性**:
    - **`ExtraHealth`**: 儲存打磨/強化等級。
    - **`ExtraEnchantment`**: 儲存玩家手動附魔的效果。
    - **`ExtraTextDisplayData`**: 儲存玩家自定義的物品名稱。

---

## 7. 總結：物品的組成清單

如果你在 C++ 中要修改一個物品，你通常在操作：
1.  **`TESFullName`**: 修改顯示名稱。
2.  **`TESModel`**: 修改外觀模型。
3.  **`TESEnchantableForm`**: 修改或添加附魔位。
4.  **`TESValueForm` / `TESWeightForm`**: 修改經濟與物理屬性。

## 8. 核心類別原始碼標註

- **`RE::TESObjectWEAP`**: `include/RE/T/TESObjectWEAP.h` - 武器。
- **`RE::TESObjectARMO`**: `include/RE/T/TESObjectARMO.h` - 護甲。
- **`RE::AlchemyItem`**: `include/RE/A/AlchemyItem.h` - 藥水。
- **`RE::ExtraDataList`**: `include/RE/E/ExtraDataList.h` - 實例差異。
