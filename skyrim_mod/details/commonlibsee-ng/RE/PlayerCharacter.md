# PlayerCharacter - 玩家角色類 (Player)
- **原始碼路徑**: `include/RE/P/PlayerCharacter.h`

`RE::PlayerCharacter` 繼承自 `RE::Character`（進而繼承自 `RE::Actor`），它是一個單例（Singleton），代表遊戲中的玩家角色。

## 獲取玩家實例
在開發插件時，通常需要隨時訪問玩家對象：
```cpp
auto player = RE::PlayerCharacter::GetSingleton();
```

## 玩家特有功能

### 1. 技能與等級 (Skills)
- **`AddSkillExperience(ActorValue, experience)`**: 手動增加某個技能（如 `kOneHanded`）的經驗值。
- **`skills` (PlayerSkills)**: 存儲了玩家各項技能的當前等級、經驗和傳奇等級（Legendary Level）。

### 2. 任務與目標 (Quests)
- **`objectives`**: 當前活躍的任務目標列表。
- **`questTargets`**: 指向地圖和羅盤上的任務標記。

### 3. 罪行系統 (Crime)
- **`crimeGoldMap`**: 存儲玩家在各個派系（Faction）中的賞金。
- **`stolenItemValueMap`**: 存儲玩家身上偷竊物品的總價值。

### 4. 遊戲狀態
- **`IsGodMode()`**: 檢查是否開啟了上帝模式。
- **`GetGrabbedRef()`**: 獲取玩家當前正用“念力”抓取的對象引用。
- **`isInThirdPersonMode`**: 檢查玩家是否處於第三人稱視角。

### 5. 跨版本兼容性
與 `Actor` 類似，`PlayerCharacter` 使用了 `GetPlayerRuntimeData()` 來獲取在不同 Skyrim 版本中可能位置變化的數據（例如 `addedPerks`, `questLog`, `playerFlags` 等）。

## 常見用法範例

```cpp
auto player = RE::PlayerCharacter::GetSingleton();

// 檢查玩家是否在潛行
if (player->IsSneaking()) {
    // ...
}

// 增加 100 點單手武器經驗
player->AddSkillExperience(RE::ActorValue::kOneHanded, 100.0f);

// 獲取玩家當前等級
auto level = player->GetLevel();
```

## 注意事項
- **單例訪問**: 雖然 `player` 指針通常在初始化後是穩定的，但最好還是在使用時通過 `GetSingleton()` 獲取，或者在腳本中保存為 `ObjectRefHandle`。
- **性能**: 玩家對象極其龐大，遍歷其 `extraList` 或 `inventory` 可能會消耗較多 CPU 週期。
