# 07. 終極挑戰：城市防禦系統 (City Defense)

身為造物主，你必須保護你的城市。本教學將展示如何監聽戰鬥事件（例如：龍來襲），並在城市周圍動態生成防禦工事（如路障或弩車）。

## 1. 核心邏輯
1.  監聽戰鬥狀態改變事件（`RE::TESCombatEvent`）。
2.  判斷發生戰鬥的地點是否在我們動態生成的城鎮範圍內。
3.  判斷威脅等級（例如：是否為巨龍）。
4.  在預設的防禦「接點 (Snap Points)」上，動態生成並激活防禦設施。

## 2. 代碼實現：巨龍來襲時的自動防禦

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

class CityDefenseManager : public RE::BSTEventSink<RE::TESCombatEvent> {
public:
    static CityDefenseManager* GetSingleton() {
        static CityDefenseManager singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*) override {
        if (!a_event || !a_event->actor || !a_event->target) return RE::BSEventNotifyControl::kContinue;

        auto aggressor = a_event->actor.get()->As<RE::Actor>();
        auto target = a_event->target.get()->As<RE::Actor>();

        // 1. 檢查戰鬥是否涉及我們的城鎮 (假設我們保存了城鎮的中心座標)
        // 這裡簡化為判斷玩家是否在場
        auto player = RE::PlayerCharacter::GetSingleton();
        if (target != player && aggressor != player) return RE::BSEventNotifyControl::kContinue;

        // 2. 判斷是否為進入戰鬥 (state == 1)
        if (a_event->newState != RE::ACTOR_COMBAT_STATE::kCombat) return RE::BSEventNotifyControl::kContinue;

        // 3. 判斷威脅：是否為龍 (Dragon)
        auto dragonRace = RE::TESForm::LookupByID<RE::TESRace>(0x00012E82); // 龍的種族 ID
        if (aggressor->GetRace() == dragonRace) {
            ActivateDefenses();
        }

        return RE::BSEventNotifyControl::kContinue;
    }

private:
    void ActivateDefenses() {
        auto player = RE::PlayerCharacter::GetSingleton();
        
        // 4. 動態生成弩車 (PlaceHolder: 0x弩車靜態對象 ID)
        auto ballistaBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00034567);
        
        if (ballistaBase) {
            // 獲取玩家周圍的幾個固定防禦點
            RE::NiPoint3 defPos1 = player->GetPosition();
            defPos1.x += 1000.0f; // 前方 1000 單位

            auto ballista = player->PlaceAtMe(ballistaBase, 1, true, false);
            ballista->SetPosition(defPos1);
            
            RE::DebugNotification("警報！偵測到巨龍！防禦弩車已部署！");
            
            // 進階：如果弩車是可交互的 (Activator)，你可以透過 C++ 強迫衛兵去操作它。
        }
    }
};
```

## 3. 進階：指揮衛兵 (Guards Control)
一旦防禦設施生成，你可以遍歷城鎮內的 NPC，找出所屬派系為「衛兵」的角色，修改他們 `AIProcess` 中的戰鬥目標（`CombatTargetSelector`，見教學 22），強迫他們集中火力攻擊巨龍，而不是四處逃竄。

---

## 4. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/T/TESCombatEvent.h` - 戰鬥狀態事件。
- `include/RE/T/TESRace.h` - 種族定義。
- `include/RE/B/BSTEvent.h` - 事件接收器。

### 推薦使用的函數
- `RE::Actor::GetRace()`：判斷攻擊者類型。
- `RE::Actor::IsInCombat()`：檢查實時戰鬥狀態。
- `RE::TESObjectREFR::PlaceAtMe(...)`：動態刷出防禦塔實體。
- `RE::Actor::StopCombat()`：強迫衛兵停止逃離並重新進入戰鬥。
