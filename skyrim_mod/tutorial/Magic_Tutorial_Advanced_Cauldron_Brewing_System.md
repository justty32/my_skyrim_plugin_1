# 實戰教學：高階大鍋釀造與法術加工系統 (Advanced Cauldron Brewing)

本教學將引導你實作一個基於物理互動與法術能量加工的動態煉金系統，讓玩家透過「投擲材料」與「施放法術」來釀造藥水。

## 難度等級與準備工作
- **難度**: 高階 (Hard)
- **準備工具**: 
    - **Creation Kit (CK)**: 用於建立基礎大鍋物件與觸發體。
    - **Visual Studio 2022**: 配合 **CommonLibSSE-NG** 開發插件。
    - **NifSkope**: 用於檢查大鍋模型的節點名稱。

---

## 實作步驟

### 步驟一：建立基礎大鍋環境
1. 在 CK 中建立一個新的 `Activator`，使用大鍋模型（如 `CraftingCookPot01.nif`）。
2. 在大鍋中心添加一個 `Trigger Volume`（觸發區域）。
3. 為該區域綁定一個腳本，或在 C++ 中透過 `RE::hkpContactListener` 監聽進入該區域的物件。

### 步驟二：實作材料偵測邏輯
當物件掉入大鍋時，我們需要辨識它是否為藥材：
1. 獲取進入觸發區的 `RE::TESObjectREFR`。
2. 檢查其基礎物件是否屬於 `RE::IngredientItem` 類別。
3. 若是，將該引用 `Disable()` 並存入大鍋的內部清單中。

### 步驟三：法術加工 Hook
透過 Hook `RE::MagicTarget::OnMagicEffectHit` 來偵測玩家對大鍋施放的法術：
1. 判斷命中的法術類型（火焰、冰霜、閃電）。
2. 根據法術屬性調整大鍋的「溫度」或「電荷」變數。
3. 播放對應的粒子效果（如冒煙、閃電火花）。

### 步驟四：成果結算與藥水生成
1. 當玩家再次與大鍋互動且滿足特定配方條件（溫度適中、材料正確）時，觸發結算。
2. 使用 `RE::IFormFactory` 動態建立一個 `RE::AlchemyItem`。
3. 將產生的藥水直接放入玩家背包。

---

## 代碼實踐 (C++ - CommonLibSSE-NG)

以下為監聽法術命中並調整溫度的核心範例：

```cpp
#include <RE/Skyrim.h>
#include <REL/Relocation.h>

struct BrewingState {
    float temperature = 20.0f;
    std::vector<RE::FormID> ingredients;
};

// 假設這是一個全局管理的大鍋狀態
BrewingState g_cauldronState;

class CauldronMagicHook {
public:
    static void Install() {
        // Hook MagicTarget 的虛函數，此處僅為邏輯示意
        // 實務上需使用 RELOCATION_ID 或 VTABLE 位址
    }

    static void OnMagicHit(RE::MagicTarget* a_target, RE::MagicItem* a_magicItem) {
        auto actor = a_target->GetTargetStatsObject();
        if (!actor || actor->GetBaseObject()->GetFormID() != CAULDRON_FORM_ID) return;

        // 檢查法術關鍵字
        if (a_magicItem->HasKeyword(RE::TESForm::LookupByID<RE::BGSKeyword>(0x43B2))) { // 火焰關鍵字
            g_cauldronState.temperature += 5.0f;
            RE::ConsoleLog::GetSingleton()->Print("大鍋溫度上升！當前: %.1f", g_cauldronState.temperature);
        } else if (a_magicItem->HasKeyword(RE::TESForm::LookupByID<RE::BGSKeyword>(0x43B3))) { // 冰霜關鍵字
            g_cauldronState.temperature -= 5.0f;
        }
    }
};
```

---

## 常見問題與驗證
- **驗證方式**: 在遊戲中朝大鍋施放火舌術，檢查控制台是否印出溫度上升的訊息。
- **問題 A**: 材料掉進去沒反應？
    - *解決*: 檢查 `hkpContactListener` 的碰撞層級，確保藥材物件的 `Layer` 能被觸發體偵測。
- **問題 B**: 效能掉幀？
    - *解決*: 僅在大鍋附近有玩家時才啟動偵測邏輯，避免全局掃描所有物理物件。
