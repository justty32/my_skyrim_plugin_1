# 08. 終極挑戰：自動化地圖標記 (Map Markers)

城鎮建好了，你該如何讓玩家可以透過世界地圖「快速旅行（Fast Travel）」回來呢？我們需要動態生成一個地圖標記（Map Marker）並設置它的顯示屬性。

## 1. 核心邏輯：操作 `ExtraMapMarker`
地圖標記在引擎中是一個看不見的 `RE::TESObjectREFR`（通常是名為 `MapMarker` 的基礎表單）。
它的靈魂在於其掛載的 `RE::ExtraMapMarker` 數據，裡面定義了標記的圖標類型（城鎮、洞穴、營地）以及是否已發現。

## 2. 代碼實現：放置城鎮標記

```cpp
#include <RE/Skyrim.h>

void CreateTownMapMarker(RE::NiPoint3 a_position, const char* a_townName) {
    auto player = RE::PlayerCharacter::GetSingleton();

    // 1. 獲取地圖標記的 BaseObject (在 Skyrim 中，MapMarker 的 FormID 固定為 0x00000010)
    auto markerBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00000010);
    if (!markerBase) return;

    // 2. 生成標記實體
    auto markerRef = player->PlaceAtMe(markerBase, 1, true, false);
    if (!markerRef) return;

    markerRef->SetPosition(a_position);

    // 3. 創建並配置 ExtraMapMarker
    auto extraMarker = new RE::ExtraMapMarker();
    auto markerData = new RE::MapMarkerData();
    
    // 設置圖標類型為 "城鎮 (City/Town)"
    markerData->flags.Set(RE::MapMarkerData::Flag::kType_City);
    
    // 設置狀態：已發現 (Visible) 且可快速旅行 (CanTravelTo)
    markerData->flags.Set(RE::MapMarkerData::Flag::kVisible);
    markerData->flags.Set(RE::MapMarkerData::Flag::kCanTravelTo);

    // 設置名稱 (這將顯示在世界地圖上)
    // 註：引擎中 MapMarkerData 的名稱設置可能涉及 TESFullName 或獨立字符串，具體視版本而定。
    // markerData->originalName = RE::BSFixedString(a_townName); // 簡化示意

    extraMarker->mapData = markerData;

    // 4. 將數據掛載到標記實體上
    markerRef->extraList.Add(extraMarker);

    RE::DebugNotification(fmt::format("地圖已更新：{} 可以進行快速旅行了。", a_townName).c_str());
}
```

## 3. 關鍵 API 標註
-   **`MapMarker (0x10)`**: 引擎中寫死的地圖標記佔位符。
-   **`RE::ExtraMapMarker`**: 儲存地圖圖標狀態的 ExtraData 容器。`include/RE/E/ExtraMapMarker.h`
-   **`RE::MapMarkerData`**: 定義圖標類型（如城鎮、礦坑）、可見度與名稱。`include/RE/M/MapMarkerData.h`

## 4. 總結
將 `MapMarker` 放置在你動態生成的城市中心（或者傳送門入口），玩家就能隨時打開 M 鍵地圖，看到這個由純代碼鍛造出來的奇蹟之城，並隨時傳送回來。

---

## 5. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/E/ExtraMapMarker.h` - 地圖標記數據。
- `include/RE/M/MapMarkerData.h` - 標記標誌與圖標類型。
- `include/RE/T/TESObjectREFR.h` - 實體屬性操作。

### 推薦使用的函數
- `RE::MapMarkerData::flags.Set(Flag)`：激活「可見」與「可傳送」狀態。
- `RE::ExtraDataList::Add(ExtraData)`：將標記邏輯掛載到隱藏實體上。
- `RE::TESObjectREFR::SetPosition(NiPoint3)`：決定地圖上標記的物理落點。
- `RE::TESForm::LookupByID<RE::TESBoundObject>(0x10)`：獲取引擎標準地圖標記模板。
