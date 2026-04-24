# TESForm - 深度解析與實戰指南

`RE::TESForm` 是 Skyrim 引擎中所有數據類別（Data Types）的根基。理解它，是編寫任何非平凡（Non-trivial）SKSE 插件的起點。

---

## 1. 核心定義與內存結構
- **TESForm 原始碼**: `include/RE/T/TESForm.h`
- **BaseFormComponent 原始碼**: `include/RE/B/BaseFormComponent.h`

`TESForm` 的內存大小為 `0x20` 字節（在 64 位系統下）。

```cpp
class TESForm : public BaseFormComponent {
public:
    // 成員變量與偏移量 (Offset)
    // 00: BaseFormComponent (包含虛函數表 VTable)
    // 08: TESFileContainer sourceFiles; // 存儲該表單來自哪些插件 (esp/esm)
    // 10: std::uint32_t formFlags;      // 狀態標記 (RecordFlags)
    // 14: FormID formID;               // 唯一 32 位 ID
    // 18: uint16_t inGameFormFlags;    // 運行時標記
    // 1A: uint8_t formType;            // 表單類型 (FormType 枚舉)
    // 1B: uint8_t pad1B;
    // 1C: uint32_t pad1C;
};
```

---

## 2. 三大核心機制

### A. FormID (唯一標識符)
每個表單都有一個 32 位的 `FormID`。
- **前兩位 (0xXXxxxxxx)**: 代表該表單所在插件的加載順序索引。
- **0xFFxxxxxx**: 代表該對象是在遊戲運行期間動態生成的（例如玩家扔在地上的蘋果、動態召喚的怪）。
- **常用靜態查找**:
  ```cpp
  auto player = RE::TESForm::LookupByID<RE::PlayerCharacter>(0x14); // 玩家引用
  auto gold = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("Gold001");
  ```

### B. FormType (類型枚舉)
`RE::FormType` 是一個龐大的枚舉，標識了表單的具體類別。
- `kActorCharacter` (62): 代表一個活著的角色。
- `kWeapon` (41): 代表武器。
- `kSpell` (22): 代表法術。
- `kReference` (61): 代表世界中的對象引用。

### C. 狀態標記 (RecordFlags)
`formFlags` 決定了對象當前的狀態，這對於判斷對象是否有效至關重要：
- `kDeleted (0x20)`: 對象已被標記為刪除。訪問已刪除的對象會導致遊戲崩潰。
- `kDisabled (0x800)`: 對象在世界中存在，但被隱藏且無交互（例如任務未開啟時的 NPC）。
- `kPersistent (0x400)`: 持久性對象，即使玩家離開單元（Cell），數據也不會被卸載。

---

## 3. 類型轉換：`As<T>()` 與 `Is()`

在 C++ 中，你經常會拿到一個 `TESForm*`，但需要將其當作 `Actor*` 使用。CommonLibSSE 模擬了 Skyrim 引擎內部的類型檢查機制。

### 安全轉換方式
```cpp
void OnInteract(RE::TESForm* a_target) {
    if (!a_target) return;

    // 1. 檢查類型
    if (a_target->Is(RE::FormType::ActorCharacter)) {
        // 2. 安全轉換
        auto actor = a_target->As<RE::Actor>();
        if (actor) {
            actor->Kill(); // 殺死該角色
        }
    }
}
```

---

## 4. 虛函數表 (VTable) 的關鍵函數

`TESForm` 定義了許多可以被子類重寫的虛函數：

- **`InitializeData()` / `ClearData()`**: 用於表單的初始化與內存清理。
- **`Load(TESFile* a_mod)`**: 從數據文件（.esp）讀取數據。
- **`SaveGame(BGSSaveFormBuffer* a_buf)`**: 存檔時調用，保存表單的動態變化。
- **`Activate(targetRef, activatorRef, ...)`**: 當玩家或 NPC 觸發/使用該對象時執行。

---

## 5. 插件開發中的實戰技巧

### 技巧 1：過濾無效對象
在遍歷對象列表時，始終檢查 `IsDeleted()`。
```cpp
if (form && !form->IsDeleted()) {
    // 執行邏輯
}
```

### 技巧 2：獲取表單所屬的插件
如果你想知道某個對象是哪個 Mod 增加的：
```cpp
auto file = form->GetFile(0); // 獲取最原始定義該表單的插件
if (file) {
    RE::ConsoleLog::GetSingleton()->Print("該對象來自插件: %s", file->fileName);
}
```

### 技巧 3：自定義 FormID 計算
對於非動態生成的對象，`FormID` 的後六位是固定的，前兩位隨加載順序變化。
如果你要查找自己插件裡的對象，需要獲取插件索引：
```cpp
auto dataHandler = RE::TESDataHandler::GetSingleton();
auto myMod = dataHandler->LookupModByName("MyAwesomeMod.esp");
if (myMod) {
    // 使用本地 ID (例如 0x123) 合成完整的 FormID
    RE::FormID fullID = myMod->GetFormID(0x123);
}
```

---

## 6. 總結
`TESForm` 是所有數據的容器。在 SKSE 開發中，你幾乎所有的操作都是在“獲取一個 Form -> 檢查類型 -> 轉換類型 -> 調用方法”這個循環中進行。它是連接 C++ 代碼與 Skyrim 靜態數據、動態世界的橋樑。
