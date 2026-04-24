# 06. 進階實戰：背包檢查與物品獎勵

本教學演示如何查詢玩家的物品欄（Inventory），並根據條件執行邏輯（添加一把鋼鐵長劍）。

## 1. 核心邏輯
1.  龍吼觸發。
2.  獲取玩家（PlayerCharacter）的物品欄數據。
3.  檢查是否存在特定物品（魯特琴）。
4.  若無，則使用 `AddObjectToContainer` 添加物品。

## 2. 代碼實現

```cpp
void CheckInventoryAndReward() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    // 1. 獲取物品欄 (一個 Map: 物品指針 -> 數量數據)
    auto inventory = player->GetInventory();
    
    bool hasLute = false;

    // 2. 遍歷背包
    for (const auto& [item, data] : inventory) {
        if (item) {
            // PlaceHolder: 0x魯特琴ID 或通過名稱判斷
            if (item->formID == 0x112233 || std::string(item->GetName()).contains("Lute")) {
                hasLute = true;
                break;
            }
        }
    }

    // 3. 根據檢查結果執行
    if (hasLute) {
        RE::DebugNotification("你已經有魯特琴了，不需要長劍。");
    } else {
        // 4. 獎勵鋼鐵長劍 (PlaceHolder ID: 0x鋼鐵長劍ID)
        auto swordBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x445566);
        if (swordBase) {
            // 添加到玩家容器
            // 參數：物品, 數量, 是否靜默(不彈出通知), 額外數據
            player->AddObjectToContainer(swordBase, nullptr, 1, nullptr);
            RE::DebugNotification("音律不足，長劍來補！已添加鋼鐵長劍。");
        }
    }
}
```

## 3. 關鍵 API 標註
-   **`GetInventory()`**: 獲取物品欄快照。`include/RE/T/TESObjectREFR.h`
-   **`AddObjectToContainer()`**: 最通用的添加物品函數。`include/RE/T/TESObjectREFR.h`

## 4. 擴展思路
-   **裝備檢查**: 你甚至可以檢查魯特琴是否「被裝備著」（使用 `entry->IsWorn()`）。
-   **數量判斷**: `data.first` 存儲了該物品的數量，你可以要求玩家必須擁有 5 個起司才能觸發效果。
