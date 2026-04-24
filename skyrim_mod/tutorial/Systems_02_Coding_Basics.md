# 02. 代碼開發實戰

現在我們已經有了項目骨架，讓我們來編寫實際的功能：**當玩家按下一個按鍵時，在屏幕上顯示玩家當前的坐標。**

## 1. 項目入口：`SKSEPluginLoad`

每個插件都必須有一個入口點。

在 `src/main.cpp` 中（或模板指定的入口文件）：
```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

// 插件加載時的入口函數
SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    // 1. 初始化 SKSE 接口
    SKSE::Init(a_skse);

    // 2. 註冊消息監聽器 (等待遊戲數據加載完成)
    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
        if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
            // 遊戲加載完成後，註冊我們的輸入事件
            RegisterInput();
        }
    });

    return true;
}
```

## 2. 實現核心功能：監聽按鍵

我們需要一個 `BSTEventSink` 來監聽按鍵。

```cpp
class MyInputHandler : public RE::BSTEventSink<RE::InputEvent*> {
public:
    static MyInputHandler* GetSingleton() {
        static MyInputHandler singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override {
        if (!a_event || !*a_event) return RE::BSEventNotifyControl::kContinue;

        for (auto event = *a_event; event; event = event->next) {
            if (event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton) {
                auto buttonEvent = event->AsButtonEvent();
                
                // 掃描碼 33 代表 'F' 鍵
                if (buttonEvent->IsDown() && buttonEvent->GetIDCode() == 33) {
                    ShowPlayerPosition();
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

// 註冊函數
void RegisterInput() {
    auto inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputDeviceManager) {
        inputDeviceManager->AddEventSink(MyInputHandler::GetSingleton());
    }
}
```

## 3. 使用 RE 層 API 獲取玩家坐標

這是展現 `CommonLibSSE-NG` 威力的地方。

```cpp
void ShowPlayerPosition() {
    // 獲取玩家單例 (include/RE/P/PlayerCharacter.h)
    auto player = RE::PlayerCharacter::GetSingleton();
    if (player) {
        // 獲取位置 (NiPoint3)
        auto pos = player->GetPosition();
        
        // 格式化訊息並顯示在屏幕通知 (include/RE/M/Misc.h)
        std::string msg = fmt::format("玩家坐標: X={:.2f}, Y={:.2f}, Z={:.2f}", pos.x, pos.y, pos.z);
        RE::DebugNotification(msg.c_str());
        
        // 同時輸出到控制台
        RE::ConsoleLog::GetSingleton()->Print(msg.c_str());
    }
}
```

## 本階段重點回顧
1. **`SKSE::Init`**: 永遠是第一步。
2. **`kDataLoaded`**: 大多數功能應該在此事件後啟動，確保遊戲世界已準備好。
3. **`GetSingleton()`**: 大多數 RE 類別都通過單例獲取實例。
4. **`RE::DebugNotification`**: 開發時最快的調試反饋方式。

> **下一步**: 前往 [階段三：構建與部署](03_Build_and_Deploy.md) 看看如何生成 DLL 並在遊戲中運行。
