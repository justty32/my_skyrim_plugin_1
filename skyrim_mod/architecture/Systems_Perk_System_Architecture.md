# Skyrim 天賦系統架構：BGSPerk, 星座介面與動態修改

Skyrim 的天賦系統（Perks）結合了後台邏輯（數據記錄）與前台視覺（3D 星座 UI）。它是玩家成長的核心動力。

---

## 1. 天賦的數據定義：`BGSPerk` (PERK)

天賦在 ESP/ESM 中由 `PERK` 記錄定義。它不僅僅是一個開關，而是一個包含條件判斷的「邏輯引擎」。
- **原始碼**: `include/RE/B/BGSPerk.h`
- **核心組件**:
    - **Conditions (條件)**: 決定天賦何時生效（例如：必須裝備單手武器，或者目標生命值低於 20%）。
    - **Perk Entries (效果項)**: 天賦具體做什麼。
        - **Modify Value**: 修改傷害、法耗、價格等數值。
        - **Add Spells**: 獲得天賦後自動學會某個法術。
        - **Ability**: 賦予一個被動的魔法效果。

---

## 2. 星座介面：視覺與資源 (The Constellations)

你看到的星空背景與星座點是由 3D 模型與 2D UI 疊加而成的。

### A. 介面資源：StatsMenu (Flash)
- **路徑**: `Data\Interface\statsmenu.swf`
- **功能**: 管理技能等級的顯示、星座點的連線邏輯。
- **原始碼**: `include/RE/S/StatsMenu.h`

### B. 3D 模型：星座背景
- **路徑**: 技能樹背景使用的是特定的 NIF 模型，通常隱藏在 `Interface` 相關的 BSA 中。
- **邏輯**: 每個「點（Node）」在 `BGSPerk` 數據中都有一個精確的座標，對應在 SWF 的坐標系中。

---

## 3. 天賦的運作流程

1.  **獲得**: 玩家在 UI 點擊，調用 `Actor::AddPerk(BGSPerk*)`。
2.  **存儲**: `Actor` 維護一個 `perkArray` 列表。
3.  **觸發**: 當引擎執行特定操作（如計算傷害）時，會調用 `RE::PerkEntry::ApplyPerk`。
    - 引擎遍歷 Actor 身上的所有 Perk。
    - 檢查 Conditions 是否滿足。
    - 滿足則應用修改（例如：傷害 * 1.2）。

---

## 4. 如何透過 C++ 插件進行修改

### A. 動態賦予/移除天賦
```cpp
void ToggleGodPerk(RE::Actor* a_player, bool a_enable) {
    auto myPerk = RE::TESForm::LookupByID<RE::BGSPerk>(0xMY_PERK_ID);
    if (a_enable) {
        a_player->AddPerk(myPerk);
    } else {
        a_player->RemovePerk(myPerk);
    }
}
```

### B. 攔截天賦效果 (進階)
你可以 Hook `RE::BGSPerk::ApplyPerk`，在原本的天賦邏輯之外注入你自己的 C++ 運算。例如：實現一個根據現實時間改變威力的天賦。

### C. 修改星座 UI
透過 Hook `StatsMenu`，你可以動態隱藏某些星座，或者在玩家滿足隱藏條件時才顯示特定的神祕天賦。

---

## 5. 核心類別原始碼標註

- **`RE::BGSPerk`**: `include/RE/B/BGSPerk.h` - 數據核心。
- **`RE::BGSPerkEntry`**: `include/RE/B/BGSPerkEntry.h` - 具體效果定義。
- **`RE::StatsMenu`**: `include/RE/S/StatsMenu.h` - 技能樹 UI 類別。
- **`RE::Actor::perkArray`**: 位於 `Actor` 類別中，儲存已習得的天賦。

---

## 6. 技術總結
1.  **想改效果**：修改 `BGSPerk` 的 `Perk Entry`。
2.  **想改視覺**：反編譯並修改 `statsmenu.swf`。
3.  **想動態接管**：Hook `AddPerk` 或 `ApplyPerk` 函數。
