# 26. 技術實戰：動態附魔與武器火焰特效

本教學展示如何透過 C++ 插件同時操作物品的「視覺層（Visuals）」與「數據層（Data）」。我們將實現：玩家施展龍吼後，右手武器燃起熊熊烈火並獲得燃燒傷害。

## 1. 核心邏輯
1.  攔截龍吼事件（見教學 04）。
2.  獲取玩家當前右手裝備的武器實體。
3.  **視覺層**: 使用 `PlayAnimation` 或掛載特效 NIF 讓武器著火。
4.  **數據層**: 透過 `ExtraEnchantment` 為該物品添加臨時附魔。

## 2. 代碼實現：附魔與點火

```cpp
#include <RE/Skyrim.h>

void IgniteRightHandWeapon() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    // 1. 獲取右手裝備的對象 (include/RE/P/PlayerCharacter.h)
    auto weaponRef = player->GetEquippedObject(false); // false 代表右手
    if (!weaponRef || !weaponRef->Is(RE::FormType::Weapon)) return;

    // 2. 數據層：添加附魔 (ExtraEnchantment)
    // 原始碼: include/RE/E/ExtraEnchantment.h
    auto inventory = player->GetInventory();
    // 獲取右手武器的 InventoryEntryData
    // (注意：這裡需要精確定位到正在裝備的那一個條目)
    
    auto fireballEnch = RE::TESForm::LookupByID<RE::EnchantmentItem>(0x0004605A); // 火焰附魔 ID
    if (fireballEnch) {
        // 為武器實例創建或獲取 ExtraDataList
        // 這裡參考教學 15：給予玩家一把強化過的劍
        auto extraEnch = new RE::ExtraEnchantment(fireballEnch, 1000); // 附魔, 電量
        // ... (將 extraEnch 加入該武器的 ExtraDataList)
    }

    // 3. 視覺層：燃起火焰
    // 方案 A: 透過施加一個隱形的魔法效果來觸發著火視覺
    // 方案 B: 直接操作武器的 3D 節點掛載粒子特效 (進階)
    
    // 這裡演示最簡單、效果最好的方案：給玩家施加一個帶有「武器藝術 (Weapon Art)」的法術
    auto fireFXSpell = RE::TESForm::LookupByID<RE::SpellItem>(0xMY_FIRE_FX_ID);
    if (fireFXSpell) {
        player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(fireFXSpell, false, player, 1.0f, false, 0.0f, player);
    }

    RE::DebugNotification("武器已注入紅蓮之力！");
}
```

## 3. 進階：直接操作模型節點掛載特效
如果你想精確控制火焰的位置，可以獲取武器的 `NiNode`。

```cpp
void AttachFireToNode(RE::Actor* a_actor) {
    auto weaponNode = a_actor->Get3D(false); // 獲取右手武器 3D
    if (weaponNode) {
        // 從資源加載火焰粒子 NIF (參考教學 Resource Formats)
        // 使用 NiNode::AttachChild() 將火焰掛載到武器的中心點
    }
}
```

## 4. 關鍵 API 標註
-   **`RE::ExtraEnchantment`**: 儲存實例附魔數據。`include/RE/E/ExtraEnchantment.h`
-   **`RE::EnchantmentItem`**: 附魔的基礎模板。`include/RE/E/EnchantmentItem.h`
-   **`RE::NiNode::AttachChild()`**: 3D 特效掛載核心。`include/RE/N/NiNode.h`

## 5. 實戰建議
-   **時效性**: `ExtraEnchantment` 添加後是永久的。如果你希望龍吼效果只持續 30 秒，你需要啟動一個計時器（Timer），在時間到後手動移除該 `ExtraData`。
-   **特效同步**: 如果你直接掛載了 `NiNode` 火焰，當玩家收起武器（Sheathe）時，記得手動銷毀火焰節點，否則火焰會懸浮在空中。
-   **原生支持**: 大多數時候，推薦在 ESP 中定義一個 Enchantment，它自帶 `Visual Effect`。C++ 只負責將這個附魔掛載到物品上，引擎會自動處理點火特效。
