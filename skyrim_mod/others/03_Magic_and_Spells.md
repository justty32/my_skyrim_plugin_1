# 03. 魔法與法術系統 (Magic & Spells)

這個系統允許你通過 C++ 觸發法術、修改魔法效果或檢查角色的魔法狀態。

## 核心類別

- **`RE::MagicCaster`**: 
  - **路徑**: `include/RE/M/MagicCaster.h`
  - 負責施放法術的對象。每個 Actor 通常都有多個 Caster（左手、右手、龍吼等）。
- **`RE::MagicTarget`**: 
  - **路徑**: `include/RE/M/MagicTarget.h`
  - 可以成為法術目標的對象（`Actor` 繼承了這個類）。
- **`RE::ActiveEffect`**: 
  - **路徑**: `include/RE/A/ActiveEffect.h`
  - 代表當前正在作用於角色身上的魔法效果（例如：正在燃燒、正在隱身）。
- **`RE::SpellItem`**: 
  - **路徑**: `include/RE/S/SpellItem.h`
  - 法術的數據表單。

## 使用範例

### 1. 讓 NPC 施放法術
你可以強制一個角色向另一個角色或方向施放魔法。

```cpp
void ForceCastSpell(RE::Actor* caster, RE::Actor* target, RE::FormID spellID) {
    auto spell = RE::TESForm::LookupByID<RE::SpellItem>(spellID);
    if (!spell || !caster) return;

    // 獲取施法者 (通常使用右手施法)
    auto magicCaster = caster->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);
    if (magicCaster) {
        // 施法 (法術, 蓄力時間, 施法者, 目標, 未知參數)
        magicCaster->CastSpellImmediate(spell, false, target, 1.0f, false, 0.0f, caster);
    }
}
```

### 2. 檢查角色是否受到特定魔法效果影響
這可以用來檢查玩家是否中毒、是否有特定的 Buff。

```cpp
bool HasMagicEffect(RE::Actor* actor, RE::FormID effectID) {
    if (!actor) return false;

    // 獲取該角色身上的所有活躍魔法效果
    auto activeEffectList = actor->AsMagicTarget()->GetActiveEffectList();
    if (!activeEffectList) return false;

    for (auto* effect : *activeEffectList) {
        if (effect && effect->GetBaseObject() && effect->GetBaseObject()->formID == effectID) {
            return true; // 找到了對應的效果
        }
    }
    return false;
}
```

### 3. 清除所有魔法效果
```cpp
void DispelAllMagic(RE::Actor* actor) {
    if (actor) {
        // 驅散所有狀態
        actor->DispelAlteredStates(RE::EffectArchetype::kNone);
    }
}
```
