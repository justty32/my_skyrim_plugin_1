# 05. 進階實戰：將木桶轉化為起司

本教學展示如何修改遊戲世界中的現有對象：將一個種類的物體（木桶）替換為另一種（起司）。

## 1. 核心邏輯
1.  檢測特定龍吼。
2.  獲取玩家準星正在看著的 `RE::TESObjectREFR`。
3.  檢查該對象的 `BaseObject` 是否為「木桶」。
4.  若是，刪除（Disable）木桶，並在原位生成起司。

## 2. 代碼實現

```cpp
void TransmuteTarget() {
    // 1. 獲取準星指向的對象 (SKSE 提供的消息接口)
    auto crosshairRef = SKSE::GetCrosshairRef(); 
    if (!crosshairRef) return;

    // 2. 獲取其基礎數據
    auto baseObj = crosshairRef->GetBaseObject();
    if (!baseObj) return;

    // 3. 判斷是否為木桶 (可以使用 FormID 或名稱判斷)
    // PlaceHolder: 0x木桶ID
    bool isBarrel = (baseObj->formID == 0xABCDE) || (std::string(baseObj->GetName()).find("Barrel") != std::string::npos);

    if (isBarrel) {
        // 獲取木桶的位置座標
        auto pos = crosshairRef->GetPosition();
        auto rot = crosshairRef->GetAngle();
        auto cell = crosshairRef->GetParentCell();

        // 4. 變魔術：隱藏並刪除原木桶
        crosshairRef->Disable();
        crosshairRef->SetDelete(true);

        // 5. 生成起司 (PlaceHolder ID: 0x起司ID)
        auto cheeseBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0xFEDCB);
        if (cheeseBase) {
            // 在木桶原位生成
            auto newCheese = crosshairRef->PlaceAtMe(cheeseBase, 1, false, false);
            RE::DebugNotification("哈瓦加！木桶變成了起司！");
        }
    } else {
        RE::DebugNotification("這不是木桶...");
    }
}
```

## 3. 關鍵 API 標註
-   **`Disable()`**: 讓對象從遊戲世界消失，但數據還在。`include/RE/T/TESObjectREFR.h`
-   **`SetDelete(true)`**: 標記為可刪除，引擎稍後會回收。
-   **`GetName()`**: 獲取顯示名稱。`include/RE/T/TESForm.h`

## 4. 注意事項
-   **容器內容物**: 如果木桶裡有東西，直接刪除木桶會導致內容物消失。高級做法是先遍歷木桶物品欄並轉移。
-   **存檔安全**: 頻繁動態生成對象（PlaceAtMe）會增大存檔體積，請謹慎使用 `SetDelete(true)` 確保回收。
