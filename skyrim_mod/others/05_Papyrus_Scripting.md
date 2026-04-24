# 05. Papyrus 腳本交互 (Papyrus Scripting)

C++ 插件通常需要與遊戲的 Papyrus 腳本協同工作。你可以從 C++ 註冊新的 Papyrus 函數（創建新的 Script API），也可以從 C++ 主動調用 Papyrus 中的函數。

## 核心類別

- **`RE::BSScript::IVirtualMachine`**: 
  - **路徑**: `include/RE/I/IVirtualMachine.h`
  - Papyrus 虛擬機接口，用於註冊函數和發起調用。
- **`RE::SkyrimVM`**: 
  - **路徑**: `include/RE/S/SkyrimVM.h`
  - Skyrim 特有的虛擬機包裝器。

## 使用範例

### 1. 註冊 C++ 函數供 Papyrus 調用
這讓 Mod 作者可以在他們的 `.psc` 腳本中直接調用你用 C++ 寫的高性能代碼。

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

// 1. 定義你的 C++ 函數
// 第一個參數總是 RE::StaticFunctionTag* (如果是靜態腳本函數)
// 或者是 RE::Actor* 等具體類別 (如果是在具體對象上的函數)
bool MyCustomPrint(RE::StaticFunctionTag*, std::string a_message) {
    SKSE::log::info("Papyrus 傳來消息: {}", a_message);
    RE::ConsoleLog::GetSingleton()->Print(a_message.c_str());
    return true;
}

// 2. 綁定函數到虛擬機
bool BindPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    // 將 C++ 函數綁定到 Papyrus 的 "MyCustomPlugin" 腳本的 "PrintMessage" 函數上
    vm->RegisterFunction("PrintMessage", "MyCustomPlugin", MyCustomPrint);
    return true;
}

// 3. 在 SKSEPluginLoad 中註冊綁定回調
SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);
    // 告訴 SKSE 在虛擬機啟動時調用我們的綁定函數
    SKSE::GetPapyrusInterface()->Register(BindPapyrusFunctions);
    return true;
}
```
**對應的 Papyrus 腳本 (`MyCustomPlugin.psc`)**:
```papyrus
ScriptName MyCustomPlugin Hidden
; 定義 Native 函數，引擎會自動轉發給 C++
Bool Function PrintMessage(String a_message) Global Native
```

### 2. 從 C++ 調用 Papyrus 函數
有時候你需要從 C++ 主動調用遊戲內現成的腳本邏輯。

```cpp
void CallPapyrusFunction() {
    auto skyrimVM = RE::SkyrimVM::GetSingleton();
    auto vm = skyrimVM ? skyrimVM->impl : nullptr;
    if (!vm) return;

    // 準備一個空的回調函數 (我們不關心返回值)
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
    
    // 準備參數
    auto args = RE::MakeFunctionArguments(std::string("Hello from C++"), 42);
    
    // 調用 "Debug" 腳本中的 "Notification" 函數
    vm->DispatchStaticCall("Debug", "Notification", RE::MakeFunctionArguments("C++ 調用 Papyrus"), callback);
}
```
