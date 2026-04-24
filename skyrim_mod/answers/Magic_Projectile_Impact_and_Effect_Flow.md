# Skyrim 架構解析：投射物碰撞、撞擊數據與效果執行流

當一支箭射中目標或一個火球在地面炸開時，引擎會經歷一套精密的「碰撞 -> 採樣 -> 負載施加」流程。本篇將詳細解析這一動態過程。

---

## 1. 碰撞偵測 (Detection)

投射物（`RE::Projectile`）在每一幀都會更新位置，並進行物理掃描。
- **碰撞層**: 投射物通常屬於 `L_PROJECTILE` 物理層。
- **觸發條件**: 當投射物的碰撞體（`bhkShape`）與另一個具備物理屬性的對象（如 `L_BIPED` 角色或 `L_STATIC` 地形）相交。
- **原始碼**: `include/RE/P/Projectile.h`

---

## 2. 撞擊處理鏈 (The Impact Chain)

一旦物理引擎偵測到碰撞，就會啟動以下邏輯鏈：

### A. 獲取撞擊數據 (BGSImpactData)
- **原始碼**: `include/RE/B/BGSImpactData.h`
- **功能**: 定義了撞擊時的視覺表現。
- **邏輯**: 引擎根據「投射物材質」與「目標表面材質」的組合，查找對應的音效（如：金屬撞擊聲）和特效（如：木屑飛濺、火花）。

### B. 爆炸觸發 (Explosion - 僅限魔法/特殊箭)
- 如果投射物定義中關聯了 `RE::BGSExplosion`：
    1.  在撞擊點 `PlaceAtMe` 一個隱形的爆炸實體。
    2.  執行球形範圍偵測（Radial Detection），尋找範圍內的所有 Actor。
    3.  對每個範圍內的目標施加負載。

---

## 3. 負載施加：箭矢 vs. 魔法火球

### A. 箭矢 (ArrowProjectile)
- **物理附著**: 如果命中的是靜態物體，引擎會將箭矢轉變為「裝飾性實體」並插在該處。
- **傷害計算**: 
    1.  發送 `TESHitEvent` 事件。
    2.  計算：`弓基礎傷害 + 箭基礎傷害 + 技能/天賦加成`。
    3.  直接修改目標的 `ActorValue::kHealth`。

### B. 魔法火球 (MissileProjectile)
- **效果傳遞**: 
    1.  獲取施法時綁定的 `SpellItem`。
    2.  將 `MagicEffect` (MGEF) 轉換為目標身上的 `RE::ActiveEffect` 實例。
- **持續性**: 即使火球消失了，目標身上的「燃燒」效果（`ActiveEffect`）依然會持續運行並每一秒調用 `Update()`。

---

## 4. 完整的運作流程圖

```text
[ Projectile::Update ] 
      |
      v
[ Havok Collision Detected ]
      |
      +--> [ BGSImpactManager ]: 播放音效與粒子 (火花、灰塵)
      |
      +--> [ Explosion::Trigger ]: 處理範圍傷害 (若是火球)
      |
      +--> [ Effect System ]: 
             |-- 物理傷害 (減 HP)
             |-- 魔法負載 (實例化 ActiveEffect)
      |
      v
[ Projectile::Dispose ]: 銷毀或停留在世界中 (箭矢)
```

---

## 5. C++ 插件開發中的介入

### A. 修改撞擊效果
你可以 Hook `BGSImpactManager::PlayImpactEffect`，讓原本普通的火球撞擊後長出一顆起司（教學 05 的變體）。

### B. 精確命中檢測
監聽 `RE::TESHitEvent` 是獲取誰擊中了誰、用了什麼武器、是否造成了暴擊的最穩定方式。

---

## 6. 核心類別原始碼標註

- **`RE::BGSImpactData`**: `include/RE/B/BGSImpactData.h` - 視覺/聽覺反應數據。
- **`RE::BGSExplosion`**: `include/RE/B/BGSExplosion.h` - 範圍負載管理器。
- **`RE::TESHitEvent`**: `include/RE/T/TESHitEvent.h` - 命中事件數據。
- **`RE::ActiveEffect`**: `include/RE/A/ActiveEffect.h` - 最終效果執行者。
