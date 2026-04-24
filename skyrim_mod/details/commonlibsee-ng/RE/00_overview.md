# CommonLibSSE RE 層概覽 (00_overview)

RE（Reverse Engineering，逆向工程）層是 CommonLibSSE 的核心。它提供了對 Skyrim 遊戲引擎內部數據結構和函數的直接 C++ 訪問。

## 工作原理

### 1. 類定義與內存佈局
RE 層中的每個類都經過精心設計，以匹配遊戲可執行文件（SkyrimSE.exe）中的內存佈局。這意味著類中的成員變量順序和大小必須與原版引擎完全一致。
- 使用 `static_assert(sizeof(ClassName) == ExpectedSize)` 來確保類的大小正確。
- 成員變量後面的註釋（如 `// 08`）通常表示該成員在類中的偏移量。

### 2. RTTI (Run-Time Type Information)
Skyrim 引擎廣泛使用 RTTI。CommonLibSSE 模擬了這一點，允許開發者在運行時安全地轉換類型。
- **`As<T>()`**: 一個模板函數，用於將一個 `TESForm*` 安全地轉換為子類（如 `Actor*`）。
- **`Is(FormType)`**: 用於檢查一個對象是否屬於特定的類型。

### 3. 虛函數表 (VTable)
引擎類通常具有虛函數。CommonLibSSE 定義了這些虛函數的順序，以便調用正確的引擎邏輯。
- 虛函數後面的註釋（如 `// 01`）表示該函數在虛函數表中的索引。

### 4. 地址重定位 (REL)
由於遊戲更新後函數的內存地址會變化，RE 層使用 `REL::Relocation`。
- **`RELOCATION_ID(SE_ID, AE_ID)`**: 通過 Address Library 的 ID 定位函數，確保跨版本兼容性。

## 命名空間
所有 RE 層的類和函數都位於 `RE` 命名空間下。
例如：`RE::TESForm`, `RE::PlayerCharacter`, `RE::Actor`。

## 如何使用
通常，你只需要在你的插件中包含 `RE/Skyrim.h`，它會自動包含大部分常用的 RE 類定義。
```cpp
#include <RE/Skyrim.h>

void MyFunction(RE::TESForm* a_form) {
    if (a_form->Is(RE::FormType::ActorCharacter)) {
        auto actor = a_form->As<RE::Actor>();
        // 對 actor 進行操作...
    }
}
```
