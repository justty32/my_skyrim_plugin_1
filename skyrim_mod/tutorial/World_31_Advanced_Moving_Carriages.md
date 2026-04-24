# 31. 終極複合實戰：巡航馬車 (動態載具與乘客掛載)

在 Skyrim 中，原版的馬車通常只是觸發一個黑屏傳送。遊戲開場那種「真正會動的馬車」是基於極其複雜的預烘焙動畫軌道（Splines）。

本教學將挑戰一項高級複合技術：**透過純 C++ 插件，打造一台可以沿著指定路徑平滑移動、上面坐著 NPC，且玩家可以隨時坐上去同行的動態馬車。**

## 1. 核心邏輯解析

這個系統由四個技術模塊組合而成：
1.  **載具本體**: 一個可以平滑移動的 `TESObjectREFR`。
2.  **動態尋路**: 讓載具沿著一系列預設座標（Waypoints）移動。
3.  **乘客綁定**: 在馬車上動態掛載「隱形座位（Furniture）」，讓 NPC 和玩家可以坐下。
4.  **座標同步**: 當馬車移動時，強制同步所有乘客的物理座標，防止他們掉下車。

## 2. 代碼實現：巡航馬車系統

### A. 生成馬車與座位

```cpp
#include <RE/Skyrim.h>

RE::TESObjectREFR* spawnedCarriage = nullptr;
RE::TESObjectREFR* playerSeat = nullptr;

void SpawnCarriage(RE::NiPoint3 a_startPos) {
    auto player = RE::PlayerCharacter::GetSingleton();
    
    // 1. 生成馬車本體 (PlaceHolder ID)
    auto carriageBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0xCARRIAGE_ID);
    spawnedCarriage = player->PlaceAtMe(carriageBase, 1, true, false);
    spawnedCarriage->SetPosition(a_startPos);

    // 2. 生成隱形座位 (PlaceHolder ID: 一個沒有模型的椅子)
    auto invisibleSeatBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0xSEAT_ID);
    playerSeat = player->PlaceAtMe(invisibleSeatBase, 1, true, false);

    // 3. 將座位掛載到馬車模型上 (參考教學 27)
    auto carriageNode = spawnedCarriage->Get3D()->AsNode();
    auto seatNode = playerSeat->Get3D();
    
    if (carriageNode && seatNode) {
        // 設置座位在馬車上的相對位置 (例如：後排座位)
        seatNode->local.translate = { 0.0f, -100.0f, 50.0f };
        carriageNode->AttachChild(seatNode, true);
    }
}
```

### B. 玩家登車 (攔截激活事件)

```cpp
class CarriageActivationHandler : public RE::BSTEventSink<RE::TESActivateEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*) override {
        if (!a_event) return RE::BSEventNotifyControl::kContinue;

        // 如果玩家點擊了我們的馬車
        if (a_event->actionRef.get() == RE::PlayerCharacter::GetSingleton() && 
            a_event->objectReference.get() == spawnedCarriage) {
            
            // 強迫玩家使用我們綁定在馬車上的座位
            auto player = RE::PlayerCharacter::GetSingleton();
            // 在 C++ 中觸發 UseFurniture 行為
            // 注意：底層依賴於 AIProcess 或 Activate 邏輯
            playerSeat->Activate(player, 0, nullptr, 1);
            
            RE::DebugNotification("你坐上了巡航馬車！");
            return RE::BSEventNotifyControl::kStop; // 攔截默認交互
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
```

### C. 平滑移動與座標同步 (Main Update Hook)

在 Skyrim 中，單純移動父節點（馬車）並不能完美帶動正在執行「坐下動畫」的 Actor，因為 Actor 的物理碰撞和動畫狀態機會強行修正他們的世界座標。我們必須使用 `TranslateTo` 進行平滑移動，並每幀同步乘客。

```cpp
void UpdateCarriageMovement() {
    if (!spawnedCarriage || !playerSeat) return;

    // 1. 定義下一個路徑點
    RE::NiPoint3 nextWaypoint = { 1000.0f, 2000.0f, 0.0f }; // 假設的目標
    
    // 2. 讓馬車平滑移動到目標點
    // TranslateTo(目標位置, 目標旋轉, 速度X, 速度Y, 速度Z, 旋轉速度)
    // 原始碼: include/RE/T/TESObjectREFR.h
    RE::NiPoint3 rot = spawnedCarriage->GetAngle();
    spawnedCarriage->TranslateTo(nextWaypoint, rot, 200.0f, 200.0f, 0.0f, 0.0f);

    // 3. 座標強制同步 (關鍵技術)
    // 雖然座位 (playerSeat) 已經掛載在馬車上，但坐在上面的玩家可能會有延遲
    auto player = RE::PlayerCharacter::GetSingleton();
    
    // 檢查玩家是否正在使用我們的座位
    // if (player->GetOccupiedFurniture() == playerSeat) {
        
        // 獲取座位當前在世界中的真實座標 (經過父節點矩陣變換後)
        auto seatWorldPos = playerSeat->Get3D()->world.translate;
        
        // 強制將玩家的物理碰撞體吸附在座位上
        player->SetPosition(seatWorldPos);
    // }
}
```

## 3. 技術難點與突破方案

### A. 為什麼原版不用這種方式？
因為 Skyrim 的 Havok 物理引擎對於「移動的平面」支持極差。如果玩家站在一塊正在 `TranslateTo` 的木板上，玩家會因為慣性滑出木板。這就是為什麼我們必須**讓玩家「坐下」**，進入傢俱動畫狀態，然後透過 C++ 強制鎖定座標。

### B. 轉彎與路徑平滑
`TranslateTo` 只能走直線。要實現沿著彎曲的道路行駛，你必須在 C++ 中定義一個**樣條曲線（Spline）**或密集的路徑點數組（Array of Waypoints），當馬車到達點 A 時，立即 `TranslateTo` 點 B，並實時計算 `rot` 讓馬車車頭轉向。

### C. NPC 車夫
與玩家登車的邏輯一致。在馬車前方掛載一個隱形座位，生成一個 NPC 並調用 `npc->UseFurniture(driverSeat)`。NPC 會保持坐姿，假裝在駕駛馬車。

## 4. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/T/TESObjectREFR.h` - 移動與激活的核心。
- `include/RE/T/TESActivateEvent.h` - 交互攔截器。
- `include/RE/N/NiTransform.h` - 世界與局部座標變換。

### 推薦使用的函數
- `RE::TESObjectREFR::TranslateTo(...)`：引擎提供的最平滑的動態移動 API。
- `RE::TESObjectREFR::Activate(...)`：模擬玩家按下 E 鍵。
- `RE::NiAVObject::world.translate`：獲取子節點（座位）經過父節點（馬車）位移後的絕對世界座標。
