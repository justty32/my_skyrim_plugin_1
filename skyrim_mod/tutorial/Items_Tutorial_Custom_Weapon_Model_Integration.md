# 實戰教學：自製武器模型整合指南 (Weapon Model Integration)

本教學將引導你如何將自製的 3D 武器模型導入 Skyrim，並設置正確的碰撞與遊戲數值。

## 難度等級與準備工作
- **難度**: 中階 (Medium)
- **準備工具**:
    - **NifSkope**: 設置 NIF 節點與材質路徑。
    - **Creation Kit (CK)**: 註冊武器 Form 與設定數值。
    - **Blender**: 製作武器模型。

---

## 實作步驟

### 步驟一：準備 NIF 模型節點
1. 在 NifSkope 中確保根節點為 `BSFadeNode`。
2. 檢查幾何體節點（`NiTriShape`），確保其包含 `BSLightingShaderProperty`。
3. **重要**: 必須包含 `bhkCollisionObject`，否則武器掉落時會穿透地板。

### 步驟二：設置特殊 Marker
1. 確保模型中心與握把位置對齊。
2. 添加 `WeaponMagicNode`：這是附魔光效產生的位置。
3. 添加 `WeaponBloodNode`：決定砍擊時血跡出現的位置。

### 步驟三：在 CK 中建立武器 Form
1. 在 `Items > Weapon` 建立新項目。
2. 在 `Model` 欄位指向你的 NIF 檔案。
3. 在 `First Person Model` 選擇一個 Static Form（通常也是相同的 NIF）。
4. 設置 `Keywords`：例如 `WeapTypeSword`，這決定了武器使用的動畫與技能加成。

### 步驟四：音效與碰撞設定
1. 設置 `Equip Slot`（右手、左手或雙手）。
2. 選擇 `Impact Data Set`（例如 `WPNMeleeSwordLight`），這決定了武器砍到不同材質時的聲音。

---

## 代碼實踐 (C++ 獲取武器數據)

如果你想在插件中動態獲取當前武器的模型路徑：

```cpp
#include <RE/Skyrim.h>

void LogWeaponModel(RE::Actor* a_actor) {
    auto weapon = a_actor->GetEquippedWeapon(false); // 獲取右手武器
    if (weapon) {
        auto modelPath = weapon->GetModel();
        RE::ConsoleLog::GetSingleton()->Print("當前裝備武器模型: %s", modelPath);
        
        // 檢查傷害值
        float damage = weapon->GetAttackDamage();
        RE::ConsoleLog::GetSingleton()->Print("基礎傷害: %.1f", damage);
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 在遊戲中按 `~` 輸入 `player.additem [ID] 1` 獲取武器，裝備並嘗試砍擊牆壁與地面。
- **問題 A**: 武器拿在手上位置偏移？
    - *解決*: 在 NifSkope 中調整模型相對於原點的座標，確保握把位於 (0,0,0)。
- **問題 B**: 附魔後全身發光而不是武器發光？
    - *解決*: 檢查 NIF 中是否缺少 `WeaponMagicNode` 節點。
- **優化提示**: 為第一人稱模型製作更高面數的 NIF，能顯著提昇視覺細節。
