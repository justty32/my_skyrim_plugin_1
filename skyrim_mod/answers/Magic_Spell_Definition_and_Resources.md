# Skyrim 法術定義與資源架構：SPEL, MGEF 與視覺載體

在 Skyrim 中，一個玩家「施放火球」的行為，實際上是多個數據記錄與資源文件協同工作的結果。本篇將解析法術的定義結構及其背後的資源檔案。

---

## 1. 核心定義：法術與效果 (The Data Pair)

法術在 ESP/ESM 中由兩層核心記錄組成：

### A. SPEL (SpellItem - 法術總控)
- **原始碼**: `include/RE/S/SpellItem.h`
- **職責**: 定義法術的學派（毀滅系、恢復系等）、法耗（Cost）、施法類型（Fire and Forget / Concentration）以及最重要的「效果清單」。
- **內容**: 一個 `SPEL` 可以包含多個效果。例如：一個法術可以同時造成「火焰傷害」和「恐懼效果」。

### B. MGEF (MagicEffect - 效果原型)
- **原始碼**: `include/RE/M/MagicEffect.h`
- **職責**: 定義效果的邏輯。它是法術的「基因」。
- **關鍵屬性**: 
    - **Archetype**: 效果的類型（如：Value Modifier, Summon, Script）。
    - **Projectile**: 關聯的投射物（見投射物架構解析）。
    - **Visual Effects**: 施法時、飛行中、命中後播放的特效 ID。

---

## 2. 視覺與資源檔案 (The Assets)

法術的視覺表現散佈在 `Data` 文件夾的不同位置：

### A. 3D 模型 (NIF)
- **路徑**: `Data\Meshes\Magic\` 或 `Data\Meshes\Effects\`。
- **內容**:
    - **Casting Art**: 手中蓄力時的模型（如：火球在手中燃燒）。
    - **Projectile Art**: 飛在空中的模型。
    - **Impact Art**: 撞擊爆炸時的模型。

### B. 貼圖與材質 (DDS)
- **路徑**: `Data\Textures\Effects\`。
- **內容**: 特效使用的噪波圖、火焰序列幀、光暈貼圖。

### C. UI 圖標 (Flash/SWF)
- **路徑**: `Data\Interface\InventoryMenu.swf` (法術在選單中的圖標)。
- **機制**: 每個法術學派對應一個固定的 SWF 圖標。

---

## 3. 法術書與習得邏輯 (Spell Books)

玩家如何學會法術？
- **類別**: `RE::TESObjectBOOK` (BOOK 記錄)。
- **關鍵字段**: `Teaches Spell`。如果這本書被定義為法術書，點擊它會觸發引擎調用 `Actor::AddSpell()`。

---

## 4. 如何透過 C++ 插件修改定義

### A. 修改法術基礎屬性
你可以透過修改 `SpellItem` 的數據成員，實現「所有法術消耗減半」或「修改法術學派」。

```cpp
void TweakSpell(RE::SpellItem* a_spell) {
    // 獲取數據結構
    auto data = a_spell->GetSpellData();
    data->cost = 0; // 變為 0 消耗法術
}
```

### B. 動態替換視覺效果
透過 Hook `MagicEffect` 的 `GetCastingArt()`，你可以讓玩家在施放普通火球時，手上顯示出神話級別的特效。

---

## 5. 核心類別原始碼標註

- **`RE::SpellItem`**: `include/RE/S/SpellItem.h` - 法術容器。
- **`RE::MagicEffect`**: `include/RE/M/MagicEffect.h` - 邏輯定義。
- **`RE::BGSProjectile`**: `include/RE/B/BGSProjectile.h` - 物理表現。
- **`RE::EffectItem`**: `include/RE/E/EffectItem.h` - SPEL 與 MGEF 的連接點。

---

## 6. 技術總結
1.  **SPEL** 決定「怎麼施放」和「包含什麼」。
2.  **MGEF** 決定「起什麼作用」和「長什麼樣」。
3.  **NIF/DDS** 提供實質的 3D 表現。
4.  **C++** 可以隨時在內存中攔截並改寫這些靜態定義。
