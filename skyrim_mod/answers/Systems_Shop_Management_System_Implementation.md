# Skyrim 商店經營系統架構 (Shop & Business Management)

要讓玩家能夠「經營商店」，而非只是向 NPC 買東西，我們必須逆向利用 Skyrim 原生的商人系統（Merchant System），並加入收益結算與店鋪升級的邏輯。

---

## 1. 原生商人系統原理 (The Merchant Chest)

Skyrim 裡的 NPC 商人其實不「攜帶」他們販售的商品。
- **Merchant Chest (商人箱子)**: 每個店鋪的地板下（Z 軸 -3000 的深處）都藏著一個專屬的寶箱。
- **Faction (商人陣營)**: NPC 被分配到 `JobMerchantFaction`。
- **連結**: NPC 的 AI 數據中連結了那個地下的商人箱子。當玩家開啟交易介面時，看到的是箱子裡的物品與金幣。

---

## 2. 實作玩家經營的商店

要讓玩家成為老闆，你需要建立一套「自動銷售」或「僱傭銷售」的機制。

### A. 建立店鋪硬體
1. **店鋪場景 (Cell)**: 在世界中建立一棟建築。
2. **存貨箱 (Inventory Chest)**: 放置一個玩家能打開的箱子，用來存放要販售的商品。
3. **金庫 (Safe/Ledger)**: 放置另一個容器或帳本，用來存放賺到的金幣。

### B. 自動銷售模擬腳本 (Economy Simulation)
由於 NPC 不會真的跑進你的店裡一件件挑選購買，我們需要用腳本「模擬」銷售過程。

- **時間推進**: 使用 `RegisterForSingleUpdateGameTime(24.0)`，每天結算一次。
- **銷售演算法**:
  ```cpp
  // 虛擬 C++ 邏輯：每日結算
  void SimulateDailySales(RE::TESObjectREFR* a_inventoryChest, RE::TESObjectREFR* a_safe) {
      auto inventory = a_inventoryChest->GetInventory();
      int totalProfit = 0;
      
      for (const auto& [item, data] : inventory) {
          // 隨機決定該物品今天是否售出 (依賴物品價值與店鋪聲望)
          if (RandomFloat(0, 100) < 20.0f) { 
              totalProfit += item->GetGoldValue();
              a_inventoryChest->RemoveItem(item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
          }
      }
      
      // 將賺取的金幣放入金庫
      if (totalProfit > 0) {
          a_safe->AddObjectToContainer(GoldForm, nullptr, totalProfit, nullptr);
      }
  }
  ```

### C. 僱傭店員 (Hiring NPCs)
- 建立一個擁有 `Sandbox` AI Package 的 NPC。
- 將他們的工作區域限制在店鋪櫃台後方。
- 當玩家將商品放入存貨箱時，這個 NPC 會成為名義上的「店長」，而腳本會在後台處理貨物減少與金錢增加。

---

## 3. 店鋪升級與視覺變化 (Visual Upgrades)

經營遊戲的樂趣在於看著破爛的店鋪變得豪華。
- **XMarker 與 Enable Parent**:
  1. 將「豪華展示櫃」與「高級地毯」等裝飾品擺放在店內。
  2. 將它們的 `Enable Parent` 設置為一個隱藏的 `XMarker`。
  3. 初始狀態將 `XMarker` 設為 `Disabled`（隱藏所有高級裝飾）。
  4. 當玩家花費金錢升級店鋪時，在代碼中呼叫 `XMarker->Enable()`，裝飾品就會瞬間出現。

---

## 4. 進階挑戰：動態定價與供需

如果你想透過 SKSE 做更深的經濟系統：
- **攔截交易介面**: Hook `RE::BarterMenu`。
- **動態調整價值**: 在玩家或 NPC 打開交易介面時，根據該地區的「供需變數」動態修改 `RE::TESForm::GetGoldValue()` 回傳的值。例如：在冬堡（Winterhold）賣火把的價格比在白漫城（Whiterun）貴三倍。

---

## 5. 核心類別原始碼標註

- **`RE::TESObjectREFR`**: 存取箱子庫存與 XMarker 狀態。
- **`RE::BarterMenu`**: `include/RE/B/BarterMenu.h` - 交易介面。
- **`RE::InventoryEntryData`**: 處理箱子內的物品堆疊與數量。

---
*文件路徑：architectures/classified/Systems/Shop_Management_System_Implementation.md*
