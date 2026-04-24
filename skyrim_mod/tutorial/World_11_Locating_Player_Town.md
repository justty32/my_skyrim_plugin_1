# 11. 技術實戰：偵測玩家所在的城鎮與區域

在製作生存類或地理相關模組時，知道玩家當前身處何處（是雪漫城還是野外？）非常重要。Skyrim 透過 `BGSLocation` 系統來管理這些信息。

## 1. 核心邏輯
1.  獲取玩家對象。
2.  透過 `GetCurrentLocation()` 獲取當前區域。
3.  遍歷區域層級（父級區域），直到找到具備「城鎮」標籤的區域。
4.  備案方案：若區域信息缺失，使用座標範圍（Bounding Box）手動判斷。

## 2. 代碼實現

```cpp
void IdentifyCurrentLocation() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    // 1. 獲取當前 Location (include/RE/B/BGSLocation.h)
    auto location = player->GetCurrentLocation();
    
    if (location) {
        // 獲取該區域的名稱
        std::string locName = location->GetFullName();
        RE::ConsoleLog::GetSingleton()->Print("當前詳細位置: %s", locName.c_str());

        // 2. 向上查找，看看是否屬於某個大城市
        // 例如：在雪漫城的“母馬橫幅”酒館內，GetCurrentLocation() 返回的是酒館，
        // 它的父級 Location 才是雪漫城。
        auto parent = location->parentLoc;
        while (parent) {
            RE::ConsoleLog::GetSingleton()->Print("父級區域: %s", parent->GetFullName());
            parent = parent->parentLoc;
        }
    } else {
        // 3. 備案方案：透過座標判斷 (粗略範例)
        auto pos = player->GetPosition();
        
        // 雪漫城大致座標範圍 (PlaceHolder)
        if (pos.x > -10000 && pos.x < 10000 && pos.y > -5000 && pos.y < 5000) {
            RE::ConsoleLog::GetSingleton()->Print("根據座標判斷，玩家可能在雪漫城附近。");
        }
    }

    // 4. 獲取當前 Cell (單元) 信息
    auto cell = player->GetParentCell();
    if (cell) {
        if (cell->IsInterior()) {
            RE::DebugNotification("玩家目前在室內。");
        }
    }
}
```

## 3. 關鍵 API 標註
-   **`GetCurrentLocation()`**: 獲取當前綁定的地理位置數據。`include/RE/P/PlayerCharacter.h`
-   **`BGSLocation`**: 存儲名稱、關鍵字（如「城鎮」、「地牢」）的對象。`include/RE/B/BGSLocation.h`
-   **`TESObjectCELL`**: 遊戲地圖的物理劃分單元。`include/RE/T/TESObjectCELL.h`

## 4. 實戰建議
-   **關鍵字過濾**: 大多數城鎮 Location 都帶有 `LocTypeTown` 或 `LocTypeCity` 關鍵字，你可以透過 `location->HasKeyword()` 來精確判斷。
-   **室內外判斷**: `cell->IsInterior()` 是判斷玩家是否在房子或地牢內的最可靠方法。
