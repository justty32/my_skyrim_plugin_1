# Skyrim 高階大鍋釀造與法術加工系統 (Advanced Cauldron Brewing)

這套系統旨在取代傳統的靜態選單煉金，轉而採用基於物理物件投入、法術能量加工（火焰、冰霜、閃電）的動態互動流程。

---

## 1. 核心組件：互動大鍋 (The Cauldron)

大鍋不僅是一個模型，它是一個具備多重偵測功能的 **容器引用 (TESObjectREFR)**。

### A. 物理碰撞偵測 (Ingredient Detection)
- **技術**: 為了偵測「投進去」的動作，我們在大鍋中心設置一個不可見的觸發體 (Trigger Volume) 或使用 `RE::hkpContactListener`。
- **邏輯**: 
    1. 當任何 `RE::TESObjectREFR` (且為 `Ingredient` 類型) 進入觸發區域。
    2. 記錄該物品的 FormID 並將其 `Disable()` (模擬沒入液體)。
    3. 在大鍋內生成對應顏色的粒子效果。

---

## 2. 法術加工邏輯 (Spell Processing)

這是系統最獨特的部分：使用法術作為加工手段。

### A. 監聽法術命中
透過 Hook `RE::MagicTarget::OnMagicEffectHit` 來偵測大鍋受到的法術影響：
- **火焰 (Fire)**: 增加「溫度」數值。過高會導致失敗（爆炸），適中則啟動反應。
- **冰霜 (Frost)**: 降低「溫度」或用於「驟冷」定型。
- **閃電 (Shock)**: 增加「能量/電荷」值。用於激發特定高等材料的活性。

### B. 狀態機 (State Machine)
大鍋會維護一組內部變量：
```cpp
struct BrewingState {
    float temperature = 20.0f; // 攝氏度
    float electricalCharge = 0.0f;
    std::vector<RE::FormID> ingredientsInPot;
    float cookingTime = 0.0f;
};
```

---

## 3. 釀造流程實作

1.  **投入期**: 玩家手動從背包扔出藥材到鍋裡。
2.  **加工期**: 
    - 玩家持續施放火焰法術。大鍋顏色隨溫度從藍 -> 綠 -> 紅轉變（使用 `NiColorInterpolator` 修改材質顏色）。
    - 若處方要求「電解」，玩家需補上一記閃電。
3.  **結算期**: 
    - 溫度與能量達到完美比例時，液體冒出金光。
    - 玩家與大鍋互動，收穫藥水。

---

## 4. 動態藥水生成 (Result Generation)

根據投入的藥材組合與加工品質，動態決定結果：

```cpp
RE::AlchemyItem* CreateResult(const BrewingState& a_state) {
    auto factory = RE::IFormFactory::GetConcreteFormFactory<RE::AlchemyItem>();
    auto potion = factory->Create();
    
    // 根據 a_state.temperature 決定藥效倍率
    float multiplier = CalculateQuality(a_state);
    
    // 遍歷所有投入的藥材，提取共同效果 (Common Effects)
    // ...
    
    return potion;
}
```

---

## 5. 技術挑戰與優化

- **液體視覺效果**: 使用 `BSEffectShaderProperty` 實作大鍋內的液體起伏與冒泡效果。
- **性能**: 不要對每一幀的所有碰撞進行檢查。僅當大鍋處於「激活狀態」時才啟動 `hkpContactListener`。
- **物理溢出**: 如果一次投入太多物品，Havok 引擎可能會崩潰。需限制大鍋同時能容納的實體數量。

---

## 6. 核心類別原始碼標註

- **`RE::MagicTarget`**: 偵測法術命中。
- **`RE::hkpContactListener`**: 偵測物理碰撞。
- **`RE::AlchemyItem`**: 藥水物件。
- **`RE::BGSExplosion`**: 用於處理煮過頭的爆炸事故。

---
*文件路徑：architectures/classified/Magic/Advanced_Cauldron_Brewing_System.md*
