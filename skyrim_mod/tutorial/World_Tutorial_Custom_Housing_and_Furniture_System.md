# 實戰教學：自定義房屋與家具製作系統 (Custom Housing & Crafting)

本教學將引導你實作一套動態房屋建設系統，讓玩家能像在《爐火》中一樣，在遊戲世界中自由放置家具。

## 難度等級與準備工作
- **難度**: 高階 (Hard)
- **準備工具**:
    - **Creation Kit (CK)**: 定義家具的 `Static` 或 `Furniture` Form。
    - **CommonLibSSE-NG**: 處理動態生成與存檔持久化。

---

## 實作步驟

### 步驟一：定義家具配方 (COBJ)
1. 在 CK 中建立 `Constructible Object`。
2. `Created Object` 選擇你要製作的家具。
3. 在 `Required Items` 中加入木材 (Wood) 或鐵錠 (Iron)。

### 步驟二：實作放置模式
1. 監聽玩家按鍵（如按住「V」鍵）。
2. 在玩家前方生成一個「半透明預覽物件」（使用 Shader 或 Alpha 屬性）。
3. 允許玩家使用滑鼠滾輪調整旋轉角度。

### 步驟三：實例生成與座標校準
1. 當玩家確認放置時，調用 `PlaceAtMe` 生成真實物件。
2. **關鍵**: 獲取地面的坡度 (Slope) 並調整家具的 Z 軸旋轉，使其與地面貼合。

### 步驟四：存檔與數據持久化
由於動態生成的物件在重啟遊戲後會消失，必須保存它們：
1. 在 `SKSE::Serialization` 中註冊數據標籤。
2. 保存每個家具的 `FormID`、`PosX/Y/Z`、`RotX/Y/Z`。
3. 加載存檔時重新生成。

---

## 代碼實踐 (C++ - CommonLibSSE-NG)

簡單的物件生成範例：

```cpp
RE::ObjectRefHandle PlaceFurniture(RE::TESForm* a_itemForm, RE::PlayerCharacter* a_player) {
    if (!a_itemForm) return RE::ObjectRefHandle();

    // 獲取玩家前方的位置
    RE::NiPoint3 pos = a_player->GetPosition();
    float angle = a_player->GetAngleZ();
    pos.x += 150.0f * sin(angle);
    pos.y += 150.0f * cos(angle);

    // 生成物件
    auto handle = a_player->PlaceAtMe(a_itemForm, 1, false, false);
    
    // 設置座標與旋轉
    if (handle) {
        handle->SetPosition(pos);
        handle->SetRotationZ(angle);
        RE::ConsoleLog::GetSingleton()->Print("家具已放置！");
    }
    
    return handle;
}
```

---

## 常見問題與驗證
- **驗證方式**: 在野外建造一個箱子，存檔並退出遊戲，重新讀檔確認箱子位置是否偏移。
- **問題 A**: 家具懸浮在空中？
    - *解決*: 實作射線檢測 (Raycast) 來獲取地面的準確 Z 座標。
- **問題 B**: NPC 會穿過家具？
    - *解決*: 對於大型家具，需使用 `RE::NavMeshObstacleManager` 動態添加導航障礙，否則 NPC 不會繞路。
