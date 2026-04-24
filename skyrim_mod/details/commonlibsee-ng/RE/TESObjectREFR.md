# TESObjectREFR - 世界引用類 (Reference)
- **原始碼路徑**: `include/RE/T/TESObjectREFR.h`

`RE::TESObjectREFR` 繼承自 `RE::TESForm`，它代表了遊戲世界中一個具體的“實例”或“引用”。

如果說 `TESForm` 是“書的模板”（例如：鐵劍的屬性），那麼 `TESObjectREFR` 就是“掉在地上或拿在手裡的那把具體的鐵劍”。

## 核心成員變量

- **`data` (OBJ_REFR)**: 
  包含引用的基本世界數據：
  - `objectReference`: 指向基礎表單（`TESBoundObject*`）的指針。
  - `location`: 座標 (`NiPoint3`)。
  - `angle`: 旋轉角度 (`NiPoint3`)。
- **`parentCell`**: 
  指向該引用當前所在的單元（`TESObjectCELL*`）。
- **`extraList` (ExtraDataList)**: 
  存儲引用的額外信息，如：
  - 物品的所有者（Ownership）。
  - 容器的鎖信息（Lock）。
  - 武器的附魔電量。
  - 獨特的名稱。
- **`loadedData`**: 
  當引用在 3D 世界中加載時（玩家在附近），此結構體不為空，包含 3D 模型（`NiAVObject*`）等信息。

## 常用功能

### 1. 位置與移動
```cpp
// 獲取位置
auto pos = ref->GetPosition();

// 移動到另一個引用附近
ref->MoveTo(playerRef);

// 設置坐標
ref->SetPosition({ 100.0f, 200.0f, 10.0f });
```

### 2. 物品欄管理 (如果引用是容器或 NPC)
```cpp
// 獲取所有物品
auto inventory = ref->GetInventory();

// 添加物品
ref->AddItem(goldForm, 100, silent);

// 移除物品
ref->RemoveItem(swordForm, 1, reason, extraList, targetRef);
```

### 3. 狀態控制
- **`Enable()` / `Disable()`**: 啟用或禁用該引用。
- **`Is3DLoaded()`**: 檢查 3D 模型是否已加載。
- **`GetDisplayFullName()`**: 獲取引用的完整顯示名稱（考慮了 ExtraData 中的自定義名稱）。

### 4. 類型檢查與轉換
```cpp
if (ref->Is(RE::FormType::ActorCharacter)) {
    auto actor = ref->As<RE::Actor>();
}
```

## 注意事項
- **引用控制代碼 (Handle)**: 在保存引用以便稍後使用時，建議使用 `RE::ObjectRefHandle` 而不是原始指針，以防止對象被刪除後指針失效。
- **Base Object vs Reference**: 始終區分 `GetBaseObject()`（獲取模板屬性）和引用本身的屬性。
