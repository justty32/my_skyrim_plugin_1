# 實戰教學：新種族建立開發指南 (New Race Creation)

本教學將教你如何從頭建立一個功能完整的自定義種族，包括骨架配置、能力設定與護甲兼容性處理。

## 難度等級與準備工作
- **難度**: 極高 (Very Hard)
- **準備工具**:
    - **Creation Kit (CK)**: 核心種族編輯器。
    - **RaceCompatibility**: 確保新種族與原版遊戲機制兼容的框架。
    - **NifSkope**: 配置種族專用骨架。

---

## 實作步驟

### 步驟一：建立 Race Form
1. 在 CK 的 `Character > Race` 中建立新項目。
2. 設置 `ID` 與 `Full Name`。
3. 在 `General Data` 設置基礎屬性（生命、魔法、耐力）。

### 步驟二：配置身體模型與骨架
1. 在 `Body Data` 標籤頁指定男性與女性的 `Skeleton` (骨架 NIF)。
2. 指定 `Body Mesh` 路徑。
3. **重要**: 如果是類人種族，建議繼承 `HumanRace` 的數據以減少重複工作。

### 步驟三：設定種族能力 (Spells/Perks)
1. 在 `Specials` 列表中加入法術（如：抗火、夜視）。
2. 在 `General Data` 設置技能等級加成（如：潛行 +10）。

### 步驟四：護甲兼容性 (Armor Addon)
1. 這是最關鍵的一步。建立一個 `ArmorAddon` 時，必須在 `Additional Races` 中加入你的新種族。
2. 建議使用 `RaceCompatibility` 模組，這可以讓你透過腳本自動讓所有護甲支持你的新種族。

---

## 代碼實踐 (C++ 檢測與修改種族)

```cpp
void AdjustRaceHeight(RE::Actor* a_actor, float a_newHeight) {
    auto race = a_actor->GetRace();
    if (race) {
        // 修改種族數據（這會影響所有該種族的 NPC！）
        race->data.height[0] = a_newHeight; // 男性身高
        race->data.height[1] = a_newHeight; // 女性身高
        
        // 僅修改單一 Actor 的縮放
        a_actor->SetObjectScale(a_newHeight);
        a_actor->Update3DModel();
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 在角色建立選單 (ShowRaceMenu) 檢查是否出現你的新種族，並確認是否能正常穿戴護甲。
- **問題 A**: 穿上衣服後身體變透明？
    - *解決*: 這是因為該護甲的 `ArmorAddon` 沒有包含你的新種族。
- **問題 B**: NPC 無法正常行走（T-Pose）？
    - *解決*: 檢查 `Skeleton` 路徑是否正確，以及動畫檔案是否與該骨架兼容。
- **技術細節**: 種族的物理碰撞大小是由 `data.height` 決定的，過大的種族可能無法通過普通的門。
 Riverside 區域的 `uGridsToLoad` 設定對巨型種族（如巨龍）的加載非常重要。
 Riverside 區域的 `uGridsToLoad` 設定對巨型種族（如巨龍）的加載非常重要。
