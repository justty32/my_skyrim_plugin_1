# 01. 事件系統 (Events)

Skyrim 引擎高度依賴事件驅動。你可以通過繼承特定的 `BSTEventSink` 來監聽遊戲中發生的幾乎任何事情。

## 核心類別與來源

- **`RE::ScriptEventSourceHolder`**: 
  - **路徑**: `include/RE/S/ScriptEventSourceHolder.h`
  - 這是一個單例（Singleton），負責分發遊戲中絕大多數與遊戲邏輯相關的事件。
- **`RE::BSTEventSink<T>`**: 
  - **路徑**: `include/RE/B/BSTEvent.h`
  - 事件接收器的模板基類。你需要繼承它並實現 `ProcessEvent` 方法。

## 常用事件類型 (`T`)

- **`RE::TESHitEvent`**: 
  - **路徑**: `include/RE/T/TESHitEvent.h`
  - 當角色被武器、法術或投射物擊中時觸發。
- **`RE::TESEquipEvent`**: 
  - **路徑**: `include/RE/T/TESEquipEvent.h`
  - 當角色裝備或卸下物品時觸發。
- **`RE::TESDeathEvent`**: 
  - **路徑**: `include/RE/T/TESDeathEvent.h`
  - 當角色死亡時觸發。
- **`RE::TESContainerChangedEvent`**: 
  - **路徑**: `include/RE/T/TESContainerChangedEvent.h`
  - 當物品在容器之間轉移（如撿起物品、存入箱子）時觸發。

## 使用範例：監聽角色被擊中事件

這個範例展示了如何記錄每次玩家被擊中的信息。

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

// 1. 創建一個繼承自 BSTEventSink 的類別
class HitEventHandler : public RE::BSTEventSink<RE::TESHitEvent> {
public:
    // 單例模式，方便註冊
    static HitEventHandler* GetSingleton() {
        static HitEventHandler singleton;
        return &singleton;
    }

    // 2. 實現 ProcessEvent 方法
    RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*) override {
        if (!a_event) return RE::BSEventNotifyControl::kContinue;

        // 獲取受擊者和攻擊者
        auto target = a_event->target.get();
        auto aggressor = a_event->cause.get();

        // 如果受擊者是玩家
        if (target && target->IsPlayerRef()) {
            if (aggressor) {
                SKSE::log::info("玩家被 {} 擊中了！", aggressor->GetName());
            }
            
            // 檢查是否被特定武器擊中
            if (a_event->source) {
                auto weapon = RE::TESForm::LookupByID<RE::TESObjectWEAP>(a_event->source);
                if (weapon) {
                    SKSE::log::info("受擊武器是: {}", weapon->GetName());
                }
            }
        }

        // 繼續傳遞事件給其他監聽者
        return RE::BSEventNotifyControl::kContinue;
    }
};

// 3. 在插件加載完成後 (kDataLoaded) 註冊該監聽器
void RegisterEvents() {
    auto eventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (eventSource) {
        eventSource->AddEventSink(HitEventHandler::GetSingleton());
        SKSE::log::info("已註冊擊中事件監聽器。");
    }
}
```
