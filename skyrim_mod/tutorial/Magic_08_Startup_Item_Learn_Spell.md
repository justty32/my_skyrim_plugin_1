# 08. 進階實戰：啟動自動贈禮與物品交互

本教學教您如何在玩家開始新遊戲或加載存檔時自動給予物品，並實現「點擊物品觸發邏輯」的功能。

## 1. 核心邏輯
1.  監聽 `SKSE::MessagingInterface::kDataLoaded`。
2.  檢查玩家背包，若無該物品則添加。
3.  監聽選單事件（`RE::MenuOpenCloseEvent`）或 Hook 物品使用邏輯。
4.  當玩家在背包中「點擊」該物品時，調用 `AddSpell`。

## 2. 代碼實現

### 第一部分：啟動自動給予物品
```cpp
void OnDataLoaded() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto itemBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0xMY_ITEM_ID);

    if (player && itemBase) {
        // 檢查玩家是否已經擁有該物品
        if (player->GetItemCount(itemBase) == 0) {
            player->AddObjectToContainer(itemBase, nullptr, 1, nullptr);
            RE::DebugNotification("獲得了神祕的奧法之石。");
        }
    }
}
```

### 第二部分：點擊物品學習技能
在純插件中，檢測「雜項 (Misc)」物品被點擊較為複雜。推薦做法是將物品定義為「書籍 (Book)」或「藥水 (Potion)」，但如果你堅持使用「雜項」，則需要監聽 UI 動作。

```cpp
// 簡單做法：監聽物品欄事件
class MyItemUsageHandler : public RE::BSTEventSink<RE::TESEquipEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*) override {
        // 雜項物品雖然不能“裝備”，但點擊時會觸發特定的通知
        // 這裡我們示範：如果該物品被點擊（觸發 Equip 事件，即使失敗）
        if (a_event->actor->IsPlayerRef() && a_event->baseObject == 0xMY_ITEM_ID) {
            LearnFireball();
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

void LearnFireball() {
    auto player = RE::PlayerCharacter::GetSingleton();
    // 火球術的 FormID (PlaceHolder)
    auto fireball = RE::TESForm::LookupByID<RE::SpellItem>(0x12F3D);

    if (player && fireball) {
        if (!player->HasSpell(fireball)) {
            player->AddSpell(fireball);
            RE::DebugNotification("你學會了火球術！");
            
            // 消耗掉這個物品 (數量 -1)
            auto itemBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0xMY_ITEM_ID);
            player->RemoveItem(itemBase, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
        }
    }
}
```

## 3. 關鍵 API 標註
-   **`AddSpell()`**: 給 Actor 添加永久法術。`include/RE/A/Actor.h`
-   **`GetItemCount()`**: 檢查背包物品數量。`include/RE/T/TESObjectREFR.h`
-   **`kDataLoaded`**: 遊戲數據（ESP/ESM）加載完成後的消息。

## 4. 實戰建議
-   **雜項物品交互**: 遊戲原生邏輯中，點擊雜項物品是不會有反應的。最標準的做法是在 ESP 中將該物品設置為 `Book` 類型，外觀設置為石頭，點擊（閱讀）後執行腳本。
-   **純插件實現**: 如果一定要雜項類型，你需要 Hook `InventoryMenu` 的消息處理函數，攔截點擊事件。對於初學者，建議先嘗試「監聽裝備事件」或「將其偽裝成藥水」。
