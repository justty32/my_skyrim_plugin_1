# 04. 進階實戰：在準星位置生成床鋪

本教學將教您如何監聽玩家的施法動作，並在玩家準星所指向的物理位置生成一個全新的建築對象（床）。

## 1. 核心邏輯流程
1.  監聽 `RE::TESSpellCastEvent` 事件。
2.  判斷施放的是否為我們的「自定義龍吼」。
3.  獲取玩家當前準星指向的引用（Reference）。
4.  在該引用的座標處生成一個床的對象。

## 2. 代碼實現

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

class ShoutSpawnHandler : public RE::BSTEventSink<RE::TESSpellCastEvent> {
public:
    static ShoutSpawnHandler* GetSingleton() {
        static ShoutSpawnHandler singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, RE::BSTEventSource<RE::TESSpellCastEvent>*) override {
        if (!a_event || !a_event->object) return RE::BSEventNotifyControl::kContinue;

        // 1. 檢查是否為玩家施放
        auto caster = a_event->object->As<RE::Actor>();
        if (!caster || !caster->IsPlayerRef()) return RE::BSEventNotifyControl::kContinue;

        // 2. 檢查龍吼 ID (PlaceHolder: 0x12345)
        // 注意：這裡應對應你在 ESP 中建立的 Spell/Shout ID
        if (a_event->spell == 0x12345) {
            SpawnBedAtCrosshair();
        }

        return RE::BSEventNotifyControl::kContinue;
    }

private:
    void SpawnBedAtCrosshair() {
        // 3. 獲取準星指向的對象
        // 我們需要監聽 CrosshairRefEvent 來獲取當前準星對象，
        // 這裡為了簡化，假設我們已經從單例獲取了當前準星指向的 Ref
        auto crosshairRef = SKSE::GetMessagingInterface()->GetCrosshairRef(); // 虛擬 API 概念

        // 正確做法：從 RE::PlayerCharacter 獲取
        auto player = RE::PlayerCharacter::GetSingleton();
        
        // 獲取當前準星指向的對象 (需要包含 SKSE/Events.h)
        // 如果準星沒指著東西，我們可以取玩家前方的位置
        RE::TESObjectREFR* targetRef = nullptr; 
        // ...（獲取邏輯）

        // 4. 生成床 (PlaceHolder ID: 0x67890 代表床的 Base Object)
        auto bedBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x67890);
        if (bedBase && player) {
            // 在玩家面前生成，或在目標位置生成
            // PlaceAtMe(對象, 數量, 是否強制持久化, 是否禁用)
            player->PlaceAtMe(bedBase, 1, false, false);
            RE::DebugNotification("床鋪已生成！");
        }
    }
};
```

## 3. 關鍵 API 標註
-   **`RE::TESSpellCastEvent`**: 監聽施法（包含龍吼）。`include/RE/T/TESSpellCastEvent.h`
-   **`PlaceAtMe`**: 在某個引用位置生成新對象。`include/RE/T/TESObjectREFR.h`

## 4. 實戰建議
-   **物理碰撞**: 床生成後，引擎會自動處理其碰撞。
-   **ESP 配合**: 你需要在 ESP 中建立一個 `Shout`，並給它一個空的 `Effect`。C++ 插件會偵測到這個 `Shout` 的施放並攔截執行邏輯。
