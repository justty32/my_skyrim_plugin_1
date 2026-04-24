# Skyrim 架構解析：物品的定義、實例化與存儲機制

在 Skyrim 中，物品（Items）的存儲遵循「**原型模式 (Prototype Pattern)**」。即：引擎只存儲一份鐵劍的藍圖，世界中成千上萬把鐵劍都指向這同一個藍圖，直到某把劍發生了特殊變化（如被玩家強化）。

---

## 1. 靜態定義：基礎表單 (Base Forms)

所有物品的原始數據都定義在 ESP/ESM 文件的特定記錄（Records）中：

### A. 武器 (WEAP - `TESObjectWEAP`)
- **定義內容**: 基礎傷害、重量、價值、攻速、模型路徑（.nif）、揮動音效。
- **原始碼**: `include/RE/T/TESObjectWEAP.h`

### B. 藥水與食物 (ALCH - `AlchemyItem`)
- **定義內容**: 魔法效果列表（`EffectItem`）、是藥水還是食物、自動計算價格標記。
- **原始碼**: `include/RE/A/AlchemyItem.h`

### C. 護甲 (ARMO - `TESObjectARMO`)
- **定義內容**: 護甲值、覆蓋部位（SlotMask）、輕/重甲類型。
- **原始碼**: `include/RE/T/TESObjectARMO.h`

---

## 2. 運行時存儲：物品欄與引用

當物品存在於世界上時，它們有兩種存在形式：

### A. 世界引用 (Reference - `TESObjectREFR`)
當一把劍掉在地上時，它是一個具備座標的實體。
- 它指向一個 `BaseObject`。
- 它擁有一份 `ExtraDataList` 用於存儲它的個體差異。

### B. 物品欄條目 (`InventoryEntryData`)
當物品在玩家或容器背包中時，為了節省內存，它們不會以 `REFR` 形式存在。
- **結構**: `InventoryEntryData` = `BaseObject*` + `Count` + `ExtraDataList*`。
- **原始碼**: `include/RE/I/InventoryEntryData.h`

---

## 3. 實例化與差異化：`ExtraDataList` (核心)

這是區分「普通鐵劍」與「精煉火傷鐵劍」的關鍵機制。如果一把物品沒有任何特殊屬性，它的 `ExtraDataList` 為空。一旦發生變化，引擎會為其附加數據：

- **強化等級**: 透過 `RE::ExtraHealth` 存儲。
- **附魔**: 透過 `RE::ExtraEnchantment` 存儲（僅限玩家手動附魔，ESP 自帶的附魔存在 BaseObject 裡）。
- **毒藥**: 透過 `RE::ExtraPoison` 存儲。
- **所有權**: 透過 `RE::ExtraOwnership` 標記該物品是否為「偷來的」。

---

## 4. 存檔 (ESS) 中的序列化

存檔時，引擎採用以下策略：

1.  **基礎物品**: 存檔只記錄一個 `FormID` 指向 ESP。
2.  **數量**: 記錄一個整數。
3.  **變化**: 如果 `ExtraDataList` 不為空，則將列表中的所有組件序列化。
    - 這就是為什麼「大量收集不同強化等級的武器」會導致存檔迅速變大，而「收集 10000 個普通金幣」幾乎不佔空間（因為金幣沒有 ExtraData）。

---

## 5. C++ 插件開發啟發

- **批量修改**: 修改 `TESObjectWEAP` 的基礎數據會立刻影響遊戲中所有該類武器。
- **精確修改**: 要修改玩家手裡的那把劍，你必須定位到 `InventoryEntryData` 並操作其 `ExtraDataList`。
- **性能優化**: 遍歷大型容器（如玩家背包）時，應優先檢查 `BaseObject` 的類型，再處理複雜的 `ExtraData`。

---

## 6. 核心類別原始碼標註

- **`RE::TESBoundObject`**: `include/RE/T/TESBoundObject.h` - 所有物品的父類。
- **`RE::InventoryChanges`**: `include/RE/I/InventoryChanges.h` - 容器物品變動管理器。
- **`RE::ExtraDataList`**: `include/RE/E/ExtraDataList.h` - 數據差異化容器。
- **`RE::AlchemyItem`**: `include/RE/A/AlchemyItem.h` - 藥水與食物定義。
