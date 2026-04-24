# 20. 高級技術：透過純插件動態創建新物品 (Dynamic Form Creation)

在 Skyrim 的引擎架構中，**物品種類 (FormType)**（例如武器、護甲、藥水）是寫死在引擎底層的枚舉，無法透過純插件憑空創造一種全新的「類別」。

然而，我們可以透過純插件**在遊戲運行時動態生成一個全新的「物品實例 (Form)」**，而不需要依賴 ESP/ESL 文件。這通常被稱為動態表單創建 (Dynamic Form Creation)。

## 1. 核心邏輯
1.  獲取對應物品種類的 **工廠類別 (IFormFactory)**。
2.  透過工廠創建一個全新的空表單。
3.  初始化並填充該物品的數據（名稱、模型、屬性）。
4.  將新物品註冊到遊戲的數據處理器中，使其可以在世界中存在。

## 2. 代碼實現：動態打造一把全新的專屬武器

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

void CreateDynamicWeapon() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    // 1. 獲取武器類別的工廠
    // 原始碼: include/RE/I/IFormFactory.h
    auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectWEAP>();
    if (!factory) return;

    // 2. 創建一個全新的空武器表單
    auto newWeapon = factory->Create();
    auto weapon = newWeapon ? newWeapon->As<RE::TESObjectWEAP>() : nullptr;
    
    if (weapon) {
        // 3. 初始化並設定基礎數據
        // 由於是空表單，我們通常需要找一個現有武器作為「模板」來複製基礎屬性
        // PlaceHolder: 0x1397E (鋼鐵長劍)
        auto templateWeapon = RE::TESForm::LookupByID<RE::TESObjectWEAP>(0x1397E);
        
        if (templateWeapon) {
            // 複製基礎模型與圖標 (TESModel)
            weapon->SetModel(templateWeapon->GetModel());
            
            // 複製裝備類型與圖標
            weapon->equipSlot = templateWeapon->equipSlot;
            
            // 設置自定義屬性
            weapon->attackDamage = 999; // 逆天傷害
            weapon->weight = 0.5f;
            weapon->value = 10000;
            
            // 設置全名 (FullName)
            // 原始碼: include/RE/T/TESFullName.h
            weapon->fullName = "程序員的憤怒";

            // 4. 將這把新武器加入玩家的背包
            player->AddObjectToContainer(weapon, nullptr, 1, nullptr);
            RE::DebugNotification("你用代碼鍛造了一把絕世好劍！");
        }
    }
}
```

## 3. 關鍵 API 標註
-   **`RE::IFormFactory`**: 表單工廠，用於在運行時分配和創建新對象的內存。`include/RE/I/IFormFactory.h`
-   **`RE::TESObjectWEAP`**: 武器數據結構。包含攻擊力、暴擊、模型等信息。`include/RE/T/TESObjectWEAP.h`
-   **`RE::TESFullName`**: 處理對象名稱的基類。`include/RE/T/TESFullName.h`

## 4. 技術難點與警告

1.  **存檔安全性 (Save Bloat)**:
    動態創建的表單 (`FormID` 通常以 `0xFF` 開頭) 會被永久保存在玩家的存檔中。如果你在每次遊戲啟動時都運行這段代碼，玩家的存檔中會充滿無用的垃圾武器。
2.  **正確的做法**:
    -   只在第一次滿足條件時創建一次。
    -   創建後，將該動態武器的 `FormID` 保存到 SKSE 的共用存儲（Co-Save / SKSE Plugin Save Data）中。
    -   下次讀取存檔時，直接透過保存的 `FormID` 來獲取該武器，而不是重新創建。
3.  **模型與紋理**:
    純插件無法憑空捏造 3D 模型 (.nif)。你只能重複使用遊戲現有的模型路徑，或者引用玩家安裝的其他 Mod 中的模型路徑。

## 5. 替代方案：修改 `ExtraData`
如果你只是想要一把「名字不同、傷害不同」的鋼鐵長劍，**極度建議**參考 `15_Advanced_Item_ExtraData.md`。使用 `ExtraDataList` 修改實例屬性比動態創建一個全新的 Base Object 更安全、更乾淨，且不會污染底層表單字典。
