# 06. 輸入系統 (Input System)

你可以監聽玩家的鍵盤、滑鼠或遊戲手柄輸入。這對於製作需要自定義快捷鍵的 Mod 非常有用。

## 核心類別

- **`RE::BSInputDeviceManager`**: 
  - **路徑**: `include/RE/B/BSInputDeviceManager.h`
  - 管理所有輸入設備的單例。
- **`RE::InputEvent`**: 
  - **路徑**: `include/RE/I/InputEvent.h`
  - 輸入事件的基類。
- **`RE::ButtonEvent`**: 
  - **路徑**: `include/RE/B/ButtonEvent.h`
  - 按鍵事件（繼承自 `InputEvent`），包含了具體按下的是哪個鍵。

## 使用範例：監聽特定的按鍵按下

這個範例展示了如何攔截鍵盤輸入。

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

class InputEventHandler : public RE::BSTEventSink<RE::InputEvent*> {
public:
    static InputEventHandler* GetSingleton() {
        static InputEventHandler singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override {
        if (!a_event || !*a_event) return RE::BSEventNotifyControl::kContinue;

        // 遍歷所有發生的輸入事件 (可能同時有多個鍵被按下)
        for (auto event = *a_event; event; event = event->next) {
            
            // 檢查是否是按鍵事件 (排除鼠標移動等)
            if (event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton) {
                auto buttonEvent = event->AsButtonEvent();
                
                // 檢查是否是鍵盤輸入
                if (buttonEvent->device.get() == RE::INPUT_DEVICE::kKeyboard) {
                    
                    auto keycode = buttonEvent->GetIDCode(); // 獲取按鍵的 DX 掃描碼
                    
                    // 檢查按鍵是否剛被按下 (IsDown) 且持續時間為 0 (防止長按重複觸發)
                    if (buttonEvent->IsDown() && buttonEvent->Value() != 0.0f && buttonEvent->HeldDuration() == 0.0f) {
                        
                        // 掃描碼 57 代表空格鍵 (Space)
                        if (keycode == 57) {
                            SKSE::log::info("玩家按下了空格鍵！");
                            RE::DebugNotification("你按下了空格！");
                        }
                    }
                }
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }
};

// 註冊輸入監聽器
void RegisterInputEvent() {
    auto inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputDeviceManager) {
        inputDeviceManager->AddEventSink(InputEventHandler::GetSingleton());
        SKSE::log::info("輸入監聽器已註冊。");
    }
}
```

**注意**: Skyrim 使用的是 **DirectX Scan Codes (DX 掃描碼)**，而不是標準的 ASCII 碼。例如：
- `W`: 17
- `A`: 30
- `S`: 31
- `D`: 32
- `Space`: 57
你可以查閱 DX Scan Codes 表來找到對應的按鍵 ID。
