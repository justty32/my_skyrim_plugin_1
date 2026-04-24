# 04. 物品欄與裝備 (Inventory & Items)

 Skyrim 中的物品系統分為 Base Object（模板，如鐵劍）和 ExtraData（實例數據，如附魔的鐵劍）。

## 核心類別

- **`RE::InventoryChanges`**: 
  - **路徑**: `include/RE/I/InventoryChanges.h`
  - 管理 NPC 或容器物品欄變化的類別。
- **`RE::InventoryEntryData`**: 
  - **路徑**: `include/RE/I/InventoryEntryData.h`
  - 物品欄中的一個條目（包含 Base Object 和該物品的實例數量、ExtraData 列表）。
- **`RE::ExtraDataList`**: 
  - **路徑**: `include/RE/E/ExtraDataList.h`
  - 附加在物品上的動態數據。

## 使用範例

### 1. 給玩家添加物品
雖然可以使用 `Actor::AddObjectToContainer`，但更簡單的方式是直接修改。

```cpp
void AddGoldToPlayer(int amount) {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto gold = RE::TESForm::LookupByID<RE::TESBoundObject>(0xF); // 金幣的 FormID 永遠是 0xF

    if (player && gold) {
        // (物品, 數量, 原因, ExtraData, 移動到目標)
        player->AddObjectToContainer(gold, nullptr, amount, nullptr);
        RE::DebugNotification(fmt::format("獲得了 {} 枚金幣", amount).c_str());
    }
}
```

### 2. 遍歷角色的物品欄並尋找裝備
這是一個非常常見的操作：獲取角色所有的物品，並找出他們正在裝備的武器。

```cpp
void CheckInventory(RE::Actor* actor) {
    if (!actor) return;

    // 獲取物品欄字典
    auto inventory = actor->GetInventory();

    for (const auto& [item, data] : inventory) {
        auto& [count, entry] = data;

        // 檢查該條目是否存在且數量大於 0
        if (count > 0 && entry) {
            // 檢查該物品是否被裝備 (isWorn)
            if (entry->IsWorn()) {
                SKSE::log::info("角色裝備了: {}, 數量: {}", item->GetName(), count);
                
                // 判斷類型
                if (item->Is(RE::FormType::Weapon)) {
                    auto weapon = item->As<RE::TESObjectWEAP>();
                    SKSE::log::info("這是一把武器，基礎傷害: {}", weapon->attackDamage);
                }
            }
        }
    }
}
```

### 3. 獲取特定部位的裝備
快速獲取角色左手或右手拿著的武器。

```cpp
auto player = RE::PlayerCharacter::GetSingleton();

// 獲取右手裝備
auto rightHand = player->GetEquippedObject(false); 
if (rightHand && rightHand->Is(RE::FormType::Weapon)) {
    // 玩家右手拿著武器
}

// 獲取頭盔
auto helmet = player->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kHair);
```
