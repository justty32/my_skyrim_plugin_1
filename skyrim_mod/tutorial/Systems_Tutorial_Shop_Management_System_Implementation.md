# 實戰教學：商店經營與模擬交易系統 (Shop & Business Management)

本教學將教你如何實作玩家擁有的店鋪，包括模擬每日銷售、庫存管理以及店員僱傭系統。

## 難度等級與準備工作
- **難度**: 高階 (Hard)
- **準備工具**:
    - **Creation Kit (CK)**: 建立店鋪單元、商人箱子與 NPC。
    - **CommonLibSSE-NG**: 編寫後台銷售模擬算法。

---

## 實作步驟

### 步驟一：建立店鋪物理架構
1. 在 CK 中建立一個室內 Cell 作為商店。
2. 在櫃檯下方放置一個 `Container`（商人箱子），這將是玩家存放貨物的地方。
3. 放置另一個安全箱 (Safe) 用於儲存利潤金幣。

### 步驟二：設置僱傭店員
1. 建立一個 NPC，為其分配 `JobMerchantFaction`。
2. 給予 NPC 一個 `Sandbox` AI Package，範圍限定在店鋪內。
3. 玩家可以透過對話招募或解僱此 NPC。

### 步驟三：實作後台模擬銷售 (核心)
由於玩家不在場時系統不會自動交易，我們需要一個計時器來模擬：
1. 每隔 24 遊戲小時執行一次結算腳本。
2. 遍歷商人箱子內的物品。
3. 根據物品價值與玩家店鋪的「聲望」計算售出機率。
4. 將售出的物品移除，並將對應金額存入安全箱。

### 步驟四：店鋪升級系統
1. 建立多組裝飾品（如招牌、豪華地毯），連結到不同的 `XMarker`。
2. 當利潤累積到一定程度後，允許玩家支付金幣「升級」，並 `Enable` 對應的 Marker。

---

## 代碼實踐 (C++ 模擬算法範例)

```cpp
void RunDailyBusiness(RE::TESObjectREFR* a_merchantChest, RE::TESObjectREFR* a_profitSafe) {
    auto inventory = a_merchantChest->GetInventory();
    float shopReputation = 0.5f; // 店鋪聲望 (0.0 - 1.0)
    int dailyProfit = 0;

    for (const auto& [item, data] : inventory) {
        // 模擬銷售機率：價值越高越難賣
        float saleChance = (100.0f / item->GetGoldValue()) * shopReputation;
        
        if (RandomFloat(0, 1) < saleChance) {
            dailyProfit += item->GetGoldValue();
            a_merchantChest->RemoveItem(item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
        }
    }

    if (dailyProfit > 0) {
        auto gold = RE::TESForm::LookupByID<RE::TESBoundObject>(0xF); // 金幣 FormID
        a_profitSafe->AddObjectToContainer(gold, nullptr, dailyProfit, nullptr);
        RE::DebugNotification("店鋪今日結算：賺取了金幣。");
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 將 10 把鐵劍放入箱子，等待 24 小時，檢查箱子內劍的數量是否減少，且安全箱內是否出現金幣。
- **問題 A**: NPC 不待在櫃檯？
    - *解決*: 在櫃檯後方放置一個 `Furniture Marker` (如：`MerchantCounter`)，並在 AI Package 中指定該點。
- **問題 B**: 賺錢太快？
    - *解決*: 加入「營運成本」邏輯，每天從利潤中扣除店員薪水與稅金。
