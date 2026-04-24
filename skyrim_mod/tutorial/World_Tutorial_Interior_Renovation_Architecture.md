# 實戰教學：建築物內部改造與室內空間設計 (Interior Renovation)

本教學將指導你如何透過動態方式改造 Skyrim 的室內單元（Interior Cell），實現如「花錢裝修」或「隨著劇情變遷」的屋內環境改變。

## 難度等級與準備工作
- **難度**: 中階 (Medium)
- **準備工具**:
    - **Creation Kit (CK)**: 用於佈置基礎家具與設置 Marker。
    - **Papyrus 腳本 / C++**: 用於切換物件狀態。

---

## 實作步驟

### 步驟一：使用 XMarker 批次管理家具
1. 在 CK 中打開你的室內 Cell。
2. 放置一個 `XMarker`，命名為 `Upgrade_Tier1_Marker`。
3. 將所有屬於「第一階段裝修」的家具選中，打開屬性視窗，在 `Enable Parent` 標籤頁連結到該 Marker。
4. 重複此步驟，建立 `Tier2` 或 `OldStuff` 的 Marker。

### 步驟二：處理導航網格 (Navmesh)
1. 針對每一組裝修方案，都需要確保 NPC 能正常行走。
2. 使用 `Navmesh Obstacle` 或在不同 Marker 下放置不同的 Navmesh 切片（進階技巧）。
3. 確保門口、床位周邊的導航連結點永遠通暢。

### 步驟三：實作切換邏輯
1. 透過腳本監聽玩家的交互（如對話或點擊帳本）。
2. 調用 `Disable()` 隱藏舊裝潢，調用 `Enable()` 顯示新裝潢。

### 步驟四：優化室內性能
1. 使用 `Room Bounds` 將大型室內空間切割為多個小區域。
2. 只有當玩家位於特定區域時，該區域內的燈光與物件才會被渲染，這對效能至關重要。

---

## 代碼實踐 (Papyrus 範例)

最簡單且穩定的 Marker 切換方式：

```papyrus
Scriptname HouseRenovationScript extends ObjectReference

ObjectReference Property OldFurnitureMarker Auto
ObjectReference Property NewFurnitureMarker Auto
Int Property RenovationCost = 500 Auto

Event OnActivate(ObjectReference akActionRef)
    If akActionRef == Game.GetPlayer()
        If Game.GetPlayer().GetGoldAmount() >= RenovationCost
            Game.GetPlayer().RemoveItem(Game.GetGold(), RenovationCost)
            
            ; 執行裝修切換
            OldFurnitureMarker.Disable()
            NewFurnitureMarker.Enable()
            
            Debug.Notification("家園裝修完成！")
        Else
            Debug.Notification("金幣不足。")
        EndIf
    EndIf
EndEvent
```

---

## 常見問題與驗證
- **驗證方式**: 在遊戲中與「裝修帳本」互動，觀察舊家具是否瞬間消失並出現新家具。
- **問題 A**: 燈光在裝修後沒變？
    - *解決*: 燈光 (Lights) 也可以設置 `Enable Parent`。確保裝修後的燈光源與新家具同步啟動。
- **問題 B**: NPC 撞到隱形的舊家具？
    - *解決*: 確保 `Disable()` 是在父 Marker 上執行的，這會同時禁用子物件的物理碰撞。
