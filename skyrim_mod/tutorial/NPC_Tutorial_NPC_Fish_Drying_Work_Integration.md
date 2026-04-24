# 步進式教學：實作 NPC 曬魚架工作行為 (Tutorial)

本教學將帶領你建立一個功能完整的曬魚架，並讓 NPC 能在白天自動前去工作。

---

## 難度等級：中等 (Intermediate)

### 準備工具：
1. **Creation Kit (CK)**: 核心編輯器。
2. **NifSkope**: 檢查家具互動點。
3. **CommonLibSSE-NG**: (選用) 用於全局行為注入。

---

## 步驟一：建立曬魚架家具 (Furniture)
1.  **建立 Form**: 在 CK 中 `WorldObjects -> Furniture` 建立新項。
2.  **指定模型**: 選擇曬魚架的 NIF 檔案。
3.  **設置互動點 (Markers)**: 
    - 確保 `Marker 0` 的座標在魚架正前方。
    - **Animation**: 勾選 `Workbench` 或 `UseIdle`。
    - **Keyword**: 新增一個自定義關鍵字 `WKS_FishRack`。

---

## 步驟二：配置 AI Package
1.  **建立 Package**: 命名為 `AAAFishDryingSandbox`。
2.  **Package Type**: 選擇 `Sandbox`。
3.  **Flags**: 勾選 `Must Reach Location`。
4.  **Procedure (核心設定)**:
    - **Type**: `Furniture`
    - **Object**: `Keyword` -> `WKS_FishRack`
    - **Radius**: `2000` (NPC 會搜尋周圍 2000 單元內的魚架)。

---

## 步驟三：設置時間與條件
1.  **Schedule**: 設定為 `8:00` 到 `18:00`。
2.  **Conditions**: 
    - `GetIsID(FishermanNPC)` (特定 NPC)。
    - 或者無條件（只要有此 Package 且周圍有魚架即觸發）。

---

## 步驟四：(進階) 使用 C++ 全局啟用
如果你想讓所有具備「農夫」特徵的 NPC 自動去曬魚：

```cpp
void OnActorLoad(RE::Actor* a_actor) {
    auto base = a_actor->GetActorBase();
    if (base && base->HasKeywordString("ActorTypeNPC")) {
        // 如果 NPC 在漁村 (Location 判定)
        if (a_actor->GetCurrentLocation()->HasKeywordString("LocTypeFishingVillage")) {
            // 動態添加 Package
            a_actor->AddPackage(g_FishRackPackage);
        }
    }
}
```

---

## 驗證方法
1.  **AI 觀測**: 在遊戲中將時間調整至中午，在漁村附近觀察 NPC 是否會主動走向曬魚架。
2.  **動畫確認**: 確認 NPC 站在正確的位置，且播放的是「掛魚」或「檢查」的動作。
3.  **控制台檢查**: 點擊 NPC 並輸入 `getisusingitem`，應回傳曬魚架的 ID。

---
*文件路徑：architectures/classified/NPC/Tutorials/Tutorial_NPC_Fish_Drying_Work_Integration.md*
