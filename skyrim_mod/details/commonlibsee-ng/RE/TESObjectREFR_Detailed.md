# TESObjectREFR 終極解析：世界引用的內存與邏輯

`RE::TESObjectREFR`（簡稱 Ref）是 Skyrim 引擎中最重要的類別。如果說 `TESForm` 是數據的靈魂，那麼 `TESObjectREFR` 就是數據在 3D 世界中的「肉身」。

---

## 1. 內存結構與原始碼路徑
- **原始碼**: `include/RE/T/TESObjectREFR.h`
- **內存大小**: 在 SE/AE 中約為 `0x98` 字節。
- **繼承鏈**: `RE::TESForm` -> `RE::TESObjectREFR`

### 核心成員偏移量 (Offsets)
- **`0x00`**: 虛函數表 (VTable)。
- **`0x14`**: FormID (32位唯一標識)。
- **`0x40`**: `RE::TESObjectCELL* parentCell` - 指向該引用當前所在的單元。
- **`0x48`**: `RE::ExtraDataList extraList` - **最核心組件**，儲存所有動態狀態。
- **`0x68`**: `RE::NiAVObject* loadedData` - 指向內存中的 3D 模型樹。如果對象未加載，此處為空。

---

## 2. 數據與實例的鏈接：`RE::OBJ_REFR`

每個 Ref 都包含一個 `data` 結構體，定義了它在世界中的物理屬性：
- **`objectReference`**: 指向它的「模板」（如：`TESObjectWEAP*` 或 `TESNPC*`）。
- **`location`**: 三維座標 (`NiPoint3`)。
- **`angle`**: 旋轉角度（弧度制）。

---

## 3. 靈魂所在：ExtraDataList (動態狀態機)

為什麼兩把同樣的鐵劍，一把有毒，一把沒毒？差別就在 `extraList`。
- **機制**: 它是一個鏈表，掛載了多種 `BSExtraData` 子類。
- **常見數據類型**:
    - **`ExtraOwnership`**: 誰擁有這個物品（決定撿起是否算「偷」）。
    - **`ExtraEnchantment`**: 玩家施加的附魔。
    - **`ExtraTextDisplayData`**: 自定義的名字。
    - **`ExtraHealth`**: 物品的強化/打磨數值。

---

## 4. 3D 模型鏈接機制 (Scene Graph Link)

Ref 與 NIF 模型之間的關係是動態的：
1.  **加載**: 當玩家靠近時，引擎讀取 NIF，將 `loadedData` 指針指向生成的 `NiNode`。
2.  **更新**: 調用 `Update3DModel()` 會重新執行此過程。
3.  **卸載**: 玩家遠離後，`loadedData` 被設為空，節點樹從內存銷毀，但 `TESObjectREFR` 對象本身依然存在。

---

## 5. 安全操作：Handle (引用控制代碼)

**警告**：在 C++ 插件中，永遠不要長時間保存 `TESObjectREFR*` 指針。
- **原因**: 引擎可能會隨時銷毀對象（如 NPC 被清理），導致指針變為「野指針」。
- **解決方案**: 使用 `RE::ObjectRefHandle`。
```cpp
// 保存方式
auto handle = myRef->CreateRefHandle();

// 使用方式
auto safeRef = handle.get(); // 如果對象已被刪除，safeRef 為空
if (safeRef) { ... }
```

---

## 6. 常用實戰函數

### A. 移動與傳送
```cpp
// 將 Ref 移動到玩家面前
a_ref->MoveTo(RE::PlayerCharacter::GetSingleton());
```

### B. 物品操作
```cpp
// 獲取該 Ref 內部的所有物品（如果它是箱子或 NPC）
auto inventory = a_ref->GetInventory();
```

### C. 啟用與禁用
```cpp
a_ref->Disable(); // 視覺消失，停止更新
a_ref->Enable();  // 重新出現
```

---

## 7. 技術總結
理解 `TESObjectREFR` 的關鍵在於明白它是一個**容器**：
- 它容器化了一個 **靜態模板** (`BaseObject`)。
- 它容器化了一組 **動態属性** (`ExtraData`)。
- 它容器化了一個 **3D 表現** (`NiAVObject`)。

操控 Skyrim 世界，本質上就是操控這些容器的屬性與狀態。
