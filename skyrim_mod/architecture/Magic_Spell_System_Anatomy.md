# Skyrim 法術系統架構：施法、數值計算與效果作用機制

Skyrim 的法術（Spells）是一個高度封裝的邏輯鏈條。它涉及施法者的屬性、天賦加成、UI 渲染以及目標的狀態改變。

---

## 1. 施法者的屬性讀取 (The Caster Side)

當你按下施法鍵時，引擎首先檢查施法者的狀態。
- **原始碼**: `include/RE/M/MagicCaster.h`
- **屬性校驗**:
    - **Magicka (法力值)**: 引擎從 `ActorValueOwner` 讀取當前法力。
    - **Cost Reduction (減耗)**: 引擎會遍歷施法者的 `perkArray`（天賦列表），尋找與當前法術學派（如：毀滅系）相關的減耗效果。
- **C++ 介入**: 你可以 Hook `RE::MagicCaster::GetSpellCost` 來動態改變任何法術的法耗。

---

## 2. 特性的動態顯示 (The UI Side)

為什麼同樣的火球術，在不同玩家手裡顯示的傷害不同？
- **動態數值渲染**:
    - **原始碼**: `include/RE/M/MagicItem.h`
    - **機制**: 物品描述中的 `<mag>` 標籤並非靜態。當你打開選單時，引擎會計算：`基礎數值 * (1 + 技能加成) * 天賦倍率`。
    - **結果**: 最終計算出的數值被轉換為字符串，替換掉 `<mag>` 顯示在 UI 上。

---

## 3. 法術影響目標的過程 (The Target Side)

法術最終透過「魔法效果（Magic Effects）」作用於目標。
- **原始碼**: `include/RE/A/ActiveEffect.h`
- **運作流程**:
    1.  **命中偵測**: 投射物（Projectile）碰撞目標（見投射物架構解析）。
    2.  **抗性檢查 (Resistance)**: 引擎檢查目標的 `ActorValue`（如：火抗、魔抗）。
    3.  **ActiveEffect 實例化**: 
        - 引擎在目標的 `RE::MagicTarget` 中創建一個新的 `ActiveEffect` 對象。
        - 啟動 `Start()` 函數（例如：播放燃燒動畫）。
    4.  **持續更新 (`Update`)**: 對於持續性傷害（DOT），引擎在每一秒調用 `ActiveEffect::Update` 扣除目標生命。
    5.  **結束銷毀 (`Finish`)**: 效果結束後，清理狀態並從列表中移除。

---

## 4. 如何透過 C++ 插件介入

### A. 修改法術效果
```cpp
void EmpowerSpell(RE::ActiveEffect* a_effect) {
    // 增加正在作用的效果威力
    a_effect->magnitude *= 2.0f;
}
```

### B. 全局施法監聽
監聽施法事件，實現「施法時隨機觸發特效」的功能。
```cpp
class SpellListener : public RE::BSTEventSink<RE::TESSpellCastEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* a_event, ...) override {
        // a_event->spell 包含施放的法術數據
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

---

## 5. 核心類別原始碼標註

- **`RE::SpellItem`**: `include/RE/S/SpellItem.h` - 法術數據定義。
- **`RE::MagicCaster`**: `include/RE/M/MagicCaster.h` - 施法行為管理。
- **`RE::MagicTarget`**: `include/RE/M/MagicTarget.h` - 受法者接口。
- **`RE::ActiveEffect`**: `include/RE/A/ActiveEffect.h` - 運行時魔法實例。

---

## 6. 技術總結
1.  **施法前**：計算成本與條件（Perks & Magicka）。
2.  **施法中**：動態計算顯示數值並渲染投射物。
3.  **命中後**：將靜態定義轉化為目標身上的 `ActiveEffect` 進行實時運算。
