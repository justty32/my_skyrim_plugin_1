# 13. 高級實戰：生成物的持久化與存檔安全

在教程 04 和 07 中，我們學會了生成對象。但在實戰中，如果處理不當，會導致存檔膨脹（Save Bloat）或對象在重啟遊戲後消失。

## 1. 難點解析
-   **持久化 (Persistence)**: 默認動態生成的對象在玩家離開單元後可能會被回收。
-   **存檔膨脹**: 如果龍吼可以無限生成房屋而不回收，存檔體積會迅速增加。

## 2. 高級解決方案：使用 `ObjectRefHandle` 與 `SetPersistent`

### 確保生成物在存檔中保留
如果你希望生成的房屋永遠存在：
```cpp
auto newHouse = player->PlaceAtMe(houseBase, 1, true, false); // 第三個參數設為 true
```

### 智能回收機制
如果你想讓生成的床在 24 小時後自動消失，你需要紀錄該引用的 `Handle`。

```cpp
// 使用 Handle 而非原始指針，防止指針懸掛 (Dangling Pointer)
RE::ObjectRefHandle spawnedBedHandle;

void UpdateCleanup() {
    auto bed = spawnedBedHandle.get();
    if (bed) {
        // 檢查遊戲時間或距離
        // ... 
        bed->Disable();
        bed->SetDelete(true); // 標記為可刪除，引擎會清理存檔中的這條記錄
    }
}
```

---

# 14. 高級實戰：安全轉化與容器轉移

在教程 05 中，我們將木桶變成了起司，但木桶裡的物品會丟失。這在高級模組中是不被允許的。

## 1. 解決方案：遍歷並轉移內容物

```cpp
void SafeTransmute(RE::TESObjectREFR* a_barrel, RE::TESBoundObject* a_cheeseBase) {
    if (!a_barrel || !a_cheeseBase) return;

    // 1. 獲取木桶的物品清單
    auto inventory = a_barrel->GetInventory();
    
    // 2. 生成起司
    auto newCheese = a_barrel->PlaceAtMe(a_cheeseBase, 1, false, false);
    
    // 3. 轉移物品
    for (auto& [item, data] : inventory) {
        auto& [count, entry] = data;
        if (count > 0) {
            // 將物品從舊木桶移動到新生成的對象（如果起司也是容器）
            // 或者直接移動給玩家
            a_barrel->RemoveItem(item, count, RE::ITEM_REMOVE_REASON::kMove, nullptr, RE::PlayerCharacter::GetSingleton());
        }
    }

    // 4. 最後再刪除木桶
    a_barrel->Disable();
    a_barrel->SetDelete(true);
}
```
