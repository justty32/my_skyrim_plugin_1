# 進階插件開發 - 事件監聽與 Papyrus 交互 (Inworld-Skyrim-Mod)

`Inworld-Skyrim-Mod` 展示了如何讓 C++ 插件與遊戲世界進行深度互動，包括監聽玩家行為、處理輸入以及與 Papyrus 腳本系統通訊。

## 1. 事件監聽 (Event Sinks)

這是監控遊戲狀態變化的最有效方式。你需要繼承 `RE::BSTEventSink<T>` 並實現 `ProcessEvent`。

### 準星指向事件 (CrosshairRefEvent)
當玩家的準星指向一個不同的對象時觸發。
```cpp
class MyEventSink : public RE::BSTEventSink<SKSE::CrosshairRefEvent> {
    RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* event, 
                                          RE::BSTEventSource<SKSE::CrosshairRefEvent>*) override {
        if (event->crosshairRef) {
            // 玩家正在看著某個東西
            auto name = event->crosshairRef->GetBaseObject()->GetName();
            RE::ConsoleLog::GetSingleton()->Print("你正在看著: %s", name);
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

### 輸入事件 (InputEvent)
直接攔截鍵盤或鼠標輸入。
```cpp
class MyInputSink : public RE::BSTEventSink<RE::InputEvent*> {
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* eventPtr, ...) override {
        auto* event = *eventPtr;
        if (event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton) {
            auto* buttonEvent = event->AsButtonEvent();
            if (buttonEvent->IsDown() && buttonEvent->GetIDCode() == 47) { // 'V' 鍵
                // 執行操作...
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

## 2. 與 Papyrus 腳本通訊

有時候你需要調用現有的 Papyrus 函數（例如調用 UI 插件 `UIExtensions`）。

### 從 C++ 調用 Papyrus 函數
```cpp
void CallPapyrus() {
    auto skyrimVM = RE::SkyrimVM::GetSingleton();
    auto vm = skyrimVM ? skyrimVM->impl : nullptr;
    if (vm) {
        RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
        auto args = RE::MakeFunctionArguments(std::string("SomeArgument"));
        // 調用名為 "MyScriptName" 的腳本中的 "MyFunctionName" 靜態函數
        vm->DispatchStaticCall("MyScriptName", "MyFunctionName", args, callback);
    }
}
```

### 發送 Mod 回調 (ModCallbackEvent)
這是一種向所有監聽該事件名（`OnModEvent`）的 Papyrus 腳本廣播消息的方式。
```cpp
SKSE::ModCallbackEvent modEvent{"MY_CUSTOM_EVENT", "StringData", 1.0f, targetActor};
SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
```

## 3. 啟動外部程序
如果你的插件需要配合外部工具運行，可以使用 Windows API。
```cpp
#include <ShellAPI.h>
void StartHelperApp() {
    ShellExecute(NULL, L"open", L"Helper.exe", NULL, NULL, SW_SHOWNORMAL);
}
```

## 總結
這個例子展示了如何打破 C++ 與遊戲腳本、外部環境之間的屏障。通過事件監聽，插件可以實時感知玩家的意圖（如看著 NPC）；通過與 Papyrus 交互，插件可以利用現有的 UI 框架（如文本輸入框）來增強遊戲體驗。
