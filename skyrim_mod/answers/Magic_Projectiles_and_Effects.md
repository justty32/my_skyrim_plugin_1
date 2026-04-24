# Skyrim 投射物與效果架構：火球、火焰與物理負載

在 Skyrim 中，當你施放一個法術時，引擎會經歷從「數據定義」到「物理實體」再到「效果施加」的複雜過程。本篇將解析投射物（Projectiles）的分類與運行原理。

---

## 1. 核心邏輯鏈條 (The Magic Pipeline)

一個法術的觸發流程如下：
**`SpellItem` (法術) -> `MagicEffect` (效果定義) -> `Projectile` (物理投射物) -> `Impact` (碰撞) -> `ActiveEffect` (作用於目標)**

### A. MagicEffect：邏輯核心
- **原始碼**: `include/RE/M/MagicEffect.h`
- 定義了法術的屬性（如：抗性檢查、傷害數值）以及它關聯的投射物類型。

### B. Projectile：物理載體
- **原始碼**: `include/RE/P/Projectile.h`
- 當法術被施放時，引擎會在世界中 `PlaceAtMe` 一個投射物實體。它是具備速度向量和物理碰撞箱的 `TESObjectREFR`。

---

## 2. 投射物的分類 (Projectile Types)

Skyrim 將投射物分為幾種不同的物理模型，定義在 `RE::BGSProjectile` 中：

### A. Missile (導彈/彈丸)
- **代表**: 火球術 (Fireball)、冰矛 (Ice Spike)。
- **特徵**: 具備初速度、重力係數和爆炸半徑。
- **類別**: `RE::MissileProjectile` (`include/RE/M/MissileProjectile.h`)。

### B. Flame (噴射/持續)
- **代表**: 噴火術 (Flames)、噴霜術 (Frostbite)。
- **特徵**: 持續性的粒子流，碰撞檢測非常頻繁（每一幀或隔幀），射程較短。
- **類別**: `RE::FlameProjectile` (`include/RE/F/FlameProjectile.h`)。

### C. Beam (光束)
- **代表**: 閃電鏈 (Chain Lightning)。
- **特徵**: 瞬間抵達目標，視覺上是一條線，物理上是一次即時的射線檢測（Raycast）。
- **類別**: `RE::BeamProjectile` (`include/RE/B/BeamProjectile.h`)。

### D. Arrow (箭矢)
- **代表**: 普通弓箭、弩箭。
- **特徵**: 受重力影響明顯，具備“穿透”或“插在目標身上”的邏輯。
- **類別**: `RE::ArrowProjectile` (`include/RE/A/ArrowProjectile.h`)。

---

## 3. 碰撞與負載 (Collision & Payload)

投射物的主要任務是將「魔法效果」運送到「目標」身上。

1.  **偵測**: 投射物每一幀更新坐標，並與環境/Actor 進行 Havok 物理碰撞檢測。
2.  **觸發**: 當發生碰撞（Impact），引擎會查找 `RE::BGSImpactData`。
3.  **施加**: 
    - 投射物銷毀（如果是 Missile）。
    - 在碰撞點觸發爆炸（Explosion）。
    - 將法術中的 `MagicEffect` 轉化為目標 `Actor` 上的 `ActiveEffect`。

---

## 4. C++ 插件開發中的控制點

### A. 修改投射物速度
你可以透過 Hook 投射物的 `Init` 函數，動態改變所有火球的飛行速度，實現“子彈時間”效果。

### B. 追蹤投射物
監聽 `RE::TESSpellCastEvent`，你可以獲取新生成的投射物 Handle，進而實現「追蹤導彈」邏輯。

```cpp
void MakeHoming(RE::Projectile* a_projectile, RE::Actor* a_target) {
    if (!a_projectile || !a_target) return;
    
    // 計算指向目標的向量
    auto targetPos = a_target->GetPosition();
    targetPos.z += 100.0f; // 指向胸口
    
    // 更新投射物的方向向量
    a_projectile->linearVelocity = CalculateVelocity(a_projectile->GetPosition(), targetPos);
}
```

---

## 5. 性能警告
- **數量限制**: 引擎對同時存在的投射物有上限（通常是數百個）。超過上限後，新生成的投射物會導致舊的直接消失。
- **持續性消耗**: `FlameProjectile` 極其消耗 CPU，因為它每秒進行多次碰撞計算。

## 6. 核心類別原始碼標註

- **`RE::BGSProjectile`**: `include/RE/B/BGSProjectile.h` - 投射物數據藍圖。
- **`RE::Projectile`**: `include/RE/P/Projectile.h` - 世界實體基類。
- **`RE::Explosion`**: `include/RE/E/Explosion.h` - 爆炸效果處理。
- **`RE::ImpactResult`**: `include/RE/I/ImpactResults.h` - 碰撞結果枚舉。
