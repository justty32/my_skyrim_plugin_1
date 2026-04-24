# Skyrim 物品文本與介紹架構：名稱、描述與書籍內容

在 Skyrim 中，物品的「文字信息」不僅僅是硬編碼的字符串，而是一套分層的緩存與翻譯系統。理解這一點可以讓你動態修改物品介紹，甚至製作自動生成的書籍。

---

## 1. 顯示名稱：`TESFullName`
幾乎所有的物品類別都繼承自 `TESFullName`。
- **原始碼**: `include/RE/T/TESFullName.h`
- **功能**: 存儲物品在物品欄中顯示的名字（如“鋼鐵長劍”）。
- **C++ 操作**: 
    ```cpp
    const char* name = item->GetName();
    // 注意：直接修改 fullName 會影響所有同類物品。
    ```

---

## 2. 物品描述：`TESDescription`
武器、護甲、法術和天賦（Perks）通常具備一段詳細介紹（如“對不死生物造成額外 10 點傷害”）。
- **原始碼**: `include/RE/T/TESDescription.h`
- **核心機制**: 描述文本通常存儲在數據文件（ESP）的 `DESC` 字段中。
- **動態獲取**:
    ```cpp
    RE::BSString description;
    item->GetDescription(description, nullptr); // 將介紹文本填入 description 變量
    ```

---

## 3. 書籍與紙條內容：`TESObjectBOOK`
書籍類物品最為特殊，因為它們包含大量的頁面內容。
- **原始碼**: `include/RE/T/TESObjectBOOK.h`
- **結構**:
    - **`RE::TESObjectBOOK::Data`**: 定義了書籍的類型（書、信件、卷軸）、技能加成、是否已讀。
    - **`itemText`**: 存儲了書籍的主體文字（HTML 標籤支持，如 `<font>`, `<br>`）。
- **動態生成內容**: 你可以在 C++ 中攔截書籍開啟事件，並將 `itemText` 替換為動態生成的字符串（例如 AI 生成的信件）。

---

## 4. 翻譯與多語言系統 (`$Strings`)
如果你看到物品名稱是以 `$` 開頭（如 `$Gold`），這代表它使用了引擎的本地化系統。
- **機制**: 引擎會去 `Data/Strings/` 文件夾下尋找對應語言的翻譯。
- **C++ 建議**: 如果你的插件要支持多語言，建議在 C++ 中通過鍵值對查找翻譯，而不是寫死中文或英文。

---

## 5. 效果介紹的特殊處理 (Magic Effects)
魔法效果的介紹通常是帶有佔位符的（如：“造成 <mag> 點傷害”）。
- **動態渲染**: 引擎會在顯示 UI 時，自動將 `<mag>` 替換為該法術實際的數值（Magnitude）。

---

## 6. 總結：如何修改物品文字？

1.  **全局修改**: 直接操作 `TESBoundObject*` 的 `TESFullName` 或 `TESDescription` 成員。
2.  **實例修改 (僅修改你手裡這把)**: 使用 `ExtraTextDisplayData` (見教學 15)，這不會影響其他同名物品。
3.  **書籍內容**: 直接操作 `TESObjectBOOK->itemText`。

## 7. 核心類別原始碼標註

- **`RE::TESFullName`**: `include/RE/T/TESFullName.h` - 名稱基類。
- **`RE::TESDescription`**: `include/RE/T/TESDescription.h` - 描述基類。
- **`RE::TESObjectBOOK`**: `include/RE/T/TESObjectBOOK.h` - 書籍特化。
- **`RE::BSString`**: `include/RE/B/BSString.h` - 引擎使用的字符串容器。
