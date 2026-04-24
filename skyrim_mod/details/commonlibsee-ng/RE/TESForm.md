# TESForm - 基礎表單類 (TESForm)
- **原始碼路徑**: `include/RE/T/TESForm.h`

`RE::TESForm` 是 Skyrim 中幾乎所有數據對象的基類。無論是一個 NPC、一把武器，還是一個法術，它們都繼承自 `TESForm`。

## 基本屬性

- **`formID` (FormID)**: 
  一個 32 位的唯一標識符。
  - 前兩位通常代表該對象所屬的插件索引（Load Order）。
  - `0xFFxxxxxx` 代表動態生成的對象。
- **`formType` (FormType)**: 
  一個枚舉值，標識該表單的具體類型（如 `Actor`, `Weapon`, `Spell` 等）。
- **`formFlags` (RecordFlags)**: 
  一組位標記，表示對象的狀態。
  - `kDeleted`: 對象已被刪除。
  - `kDisabled`: 對象在遊戲中被禁用。
  - `kPersistent`: 持久性對象，不會被卸載。

## 核心功能

### 1. 查找對象
你可以通過靜態方法快速找到遊戲中的對象：
```cpp
// 通過 ID 查找
auto form = RE::TESForm::LookupByID(0x00000007); // 查找玩家 (Player)

// 通過 EditorID 查找
auto gold = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("Gold001");
```

### 2. 類型轉換
使用 `As<T>()` 進行安全轉換：
```cpp
if (form->Is(RE::FormType::ActorCharacter)) {
    auto actor = form->As<RE::Actor>();
}
```

### 3. 表單來源
- **`sourceFiles`**: 存儲了修改過此表單的插件列表。
- **`GetFile()`**: 獲取定義或最後修改此表單的 `TESFile`。

## 常用成員函數

- **`GetName()`**: 獲取對象的顯示名稱（如果有的話）。
- **`GetGoldValue()`**: 獲取對象的金幣價值。
- **`GetWeight()`**: 獲取對象的重量。
- **`Activate()`**: 虛擬函數，觸發對象的交互邏輯。

## 注意事項
`TESForm` 本身不包含空間信息（如座標）。如果你需要處理遊戲世界中的實體對象，通常需要查看其子類 `TESObjectREFR`（引用）。
