# Actor - 角色類 (Actor)
- **原始碼路徑**: `include/RE/A/Actor.h`

`RE::Actor` 繼承自 `RE::TESObjectREFR`，是遊戲中所有生物（NPC、怪物、玩家）的基類。它是 Skyrim 中最複雜、功能最豐富的類之一。

## 核心能力

### 1. 屬性值 (Actor Values)
管理生命值（Health）、魔法值（Magicka）、體力值（Stamina）以及各類技能等級。
- **`GetActorValue(ActorValue)`**: 獲取當前值。
- **`SetActorValue(ActorValue, value)`**: 設置基礎值。
- **`RestoreActorValue(modifier, ActorValue, value)`**: 恢復屬性值（例如喝藥水）。

### 2. AI 與 行為
- **`currentProcess` (AIProcess)**: 處理角色的當前動作、目標、路徑規劃等。
- **`EvaluatePackage()`**: 強制重新評估角色的行為包（Package）。
- **`IsInCombat()`**: 檢查角色是否處於戰鬥狀態。

### 3. 戰鬥與魔法
- **`GetCombatTarget()`**: 獲取當前的戰鬥目標。
- **`AddSpell(SpellItem*)`**: 給角色添加法術。
- **`IsCasting(MagicItem*)`**: 檢查是否正在施法。

### 4. 裝備與物品欄
- **`GetEquippedObject(leftHand)`**: 獲取裝備的武器或法術。
- **`GetWornArmor(slot)`**: 獲取特定部位穿戴的護甲。
- **`Update3DModel()`**: 當裝備改變時更新角色的 3D 模型。

## 狀態標誌 (Flags)
`Actor` 有大量的狀態標誌，分佈在 `boolBits` 和 `boolFlags` 中：
- `IsDead()`: 是否死亡。
- `IsSneaking()`: 是否在潛行。
- `IsEssential()`: 是否為無敵（重要）NPC。
- `IsPlayerTeammate()`: 是否為玩家隊友。

## 常用成員函數

- **`Kill()` / `Resurrect()`**: 殺死或復活角色。
- **`HasPerk(BGSPerk*)`**: 檢查是否具有特定天賦。
- **`GetRace()`**: 獲取角色的種族。

## 注意事項
由於 `Actor` 的數據結構非常龐大，CommonLibSSE NG 使用了 `GetActorRuntimeData()` 來訪問在不同版本（SE/AE）中可能發生偏移變化的成員變量。
