# Skyrim 腳本綁定架構：C++ 與 Papyrus 的橋樑

在 Skyrim 中，Papyrus 腳本雖然靈活但性能有限。透過 SKSE，我們可以將 C++ 編寫的高性能函數「導出」給 Papyrus 調用。這些函數被稱為 **Native Functions**。

---

## 1. 核心組件：Papyrus 虛擬機 (PVM)

- **類別**: `RE::BSScript::IVirtualMachine`
- **原始碼**: `include/RE/I/IVirtualMachine.h`
- **職責**: 負責解析 `.pex` 字節碼、管理變量堆棧以及處理與 C++ 插件的交互請求。

---

## 2. 註冊機制 (The Registration Process)

要將 C++ 功能綁定到腳本，必須遵循以下流程：

### A. 定義 C++ 函數
函數的第一個參數通常是 `RE::StaticFunctionTag*`（靜態函數）或具體的類別指針（實例函數）。

```cpp
// 一個簡單的 C++ 函數，計算兩個數的和
int AddNumbers(RE::StaticFunctionTag*, int a, int b) {
    return a + b;
}
```

### B. 綁定函數
透過 `IVirtualMachine::RegisterFunction` 進行註冊。
- **原始碼**: `include/RE/B/BSScript/NativeFunction.h`
- **參數**:
    1.  `functionName`: 在腳本中調用的名字。
    2.  `className`: 腳本文件的名字（不含後綴）。
    3.  `functionPtr`: 你的 C++ 函數地址。

### C. SKSE 接口接入
你不能在任何時候註冊函數，必須在 SKSE 發送 `kMsg_PapyrusSetup` 消息時進行。

```cpp
bool BindFunctions(RE::BSScript::IVirtualMachine* a_vm) {
    a_vm->RegisterFunction("AddNumbers", "MyScript", AddNumbers);
    return true;
}

// 在 SKSEPluginLoad 中：
SKSE::GetPapyrusInterface()->Register(BindFunctions);
```

---

## 3. 數據類型映射 (Type Mapping)

引擎會自動在 C++ 與 Papyrus 類型之間進行轉換：

| Papyrus 類型 | C++ 類型 | 說明 |
| :--- | :--- | :--- |
| **Int** | `std::int32_t` | 32 位整數。 |
| **Float** | `float` | 浮點數。 |
| **String** | `std::string` 或 `RE::BSFixedString` | 建議使用 `BSFixedString` 以提高效率。 |
| **Object** | `RE::TESForm*` 或其子類 | 如 `RE::Actor*`, `RE::TESObjectREFR*`。 |
| **Array** | `std::vector<T>` 或 `RE::BSTArray<T>` | 陣列轉換開銷較大。 |

---

## 4. 實例函數 vs 靜態函數

- **Static (靜態)**: 
    - 腳本宣告: `Function MyFunc() Global Native`
    - C++ 第一參數: `RE::StaticFunctionTag*`
- **Instance (實例)**: 
    - 腳本宣告: `Function MyFunc() Native` (必須附加在對應類別的腳本上)
    - C++ 第一參數: 對應的類別指針（如 `RE::Actor* a_this`）。這讓你可以直接操作調用該函數的 NPC 實體。

---

## 5. 技術難點：異步與回調 (Latency)

- **阻塞性**: Native 函數在 C++ 主線程中執行。如果你的 C++ 代碼運行太久，會導致遊戲掉幀。
- **回調**: 如果你需要執行耗時操作（如聯網，見 `Inworld` 範例），你必須使用 `RE::BSScript::IStackCallbackFunctor` 異步返回結果給 Papyrus。

---

## 6. 核心類別原始碼標註

- **`RE::BSScript::IVirtualMachine`**: `include/RE/I/IVirtualMachine.h` - 虛擬機接口。
- **`RE::BSScript::Internal::VirtualMachine`**: `src/RE/V/VirtualMachine.cpp` - 引擎內部的具體實現。
- **`SKSE::PapyrusInterface`**: `include/SKSE/Interfaces.h` - SKSE 提供的註冊入口。
- **`RE::StaticFunctionTag`**: `include/RE/S/StaticFunctionTag.h` - 靜態函數標記位。
