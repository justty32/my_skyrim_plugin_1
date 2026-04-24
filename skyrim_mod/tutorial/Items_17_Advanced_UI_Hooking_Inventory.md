# 17. 高級實戰：深層 UI Hooking (讓雜項物品可點擊)

在教程 08 中，我們提到「雜項」物品默認無法交互。現在我們透過 Hook 物品欄選單來徹底解決它。

## 1. 核心邏輯
1.  找到物品欄選單（`InventoryMenu`）處理按鍵按下的函數地址。
2.  安裝一個 `Thunk` (跳板)。
3.  在 Thunk 中獲取當前選中的物品。
4.  如果是我們的「奧法之石」，執行自定義邏輯並攔截（Consume）該點擊事件。

## 2. 代碼實現 (偽代碼與 ID 標記)

```cpp
struct Inventory_ProcessMessage {
    static bool thunk(RE::InventoryMenu* a_menu, RE::UIMessage* a_message) {
        // 檢查是否為物品選中/點擊事件 (kInventory_SelectItem)
        if (a_message->type == RE::UI_MESSAGE_TYPE::kUserEvent) {
            // 獲取當前選中的條目
            auto entryData = a_menu->GetSelectedData();
            if (entryData && entryData->object->formID == 0xMY_ITEM_ID) {
                // 執行學習火球術邏輯
                LearnFireball();
                return true; // 返回 true 攔截事件，防止遊戲彈出“無法裝備”的提示
            }
        }
        return func(a_menu, a_message);
    }
    static inline REL::Relocation<decltype(thunk)> func;
};

// 安裝 Hook (原始碼參考教程 01_hooking_thunks.md)
void InstallUIHook() {
    // 找到 InventoryMenu::ProcessMessage 的地址 (ID 51912 等)
    // REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_InventoryMenu[0] };
    // stl::write_vfunc<0x...>(vtable, Inventory_ProcessMessage::thunk);
}
```

## 3. 總結
透過這種方式，你可以讓遊戲中的任何東西變得可以交互，甚至是在原本不允許點擊的界面（如讀取界面、載入界面）添加功能。
