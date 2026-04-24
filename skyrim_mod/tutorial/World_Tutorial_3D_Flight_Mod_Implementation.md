# 步進式教學：實作翅膀模組與 3D 飛行功能 (Tutorial)

本教學將引導你完成從翅膀附加到實作 SKSE 自由飛行的完整流程。

---

## 難度等級：專家 (Expert)

### 準備工具：
1. **翅膀模型 (NIF)**: 具備骨架權重。
2. **CommonLibSSE-NG**: 用於編寫飛行邏輯。
3. **Open Animation Replacer (OAR)**: 用於飛行手勢。

---

## 步驟一：翅膀附加 (Rigging)
1.  **NIF 設置**: 將翅膀模型的根節點名稱改為 `NPC Spine2 [Spn2]`。
2.  **遊戲內加載**: 
    - 建立一個 `ArmorAddon`。
    - 在腳本或 C++ 中使用 `EquipItem` 讓玩家穿上翅膀。

---

## 步驟二：攔截輸入與起飛邏輯
監聽跳躍鍵（Space）的長按。

```cpp
void OnUpdate(RE::PlayerCharacter* a_player) {
    if (IsSpacePressed() && a_player->IsFalling()) {
        // 觸發起飛
        EnterFlightMode(a_player);
    }
}
```

---

## 步驟三：實作 3D 向量移動 (核心代碼)
在每一幀更新玩家座標：

```cpp
void UpdateFlightPosition(RE::PlayerCharacter* a_player, float a_deltaTime) {
    auto camera = RE::PlayerCamera::GetSingleton();
    RE::NiPoint3 direction;
    
    // 獲取鏡頭朝向
    camera->Update();
    camera->GetCameraRuntimeData().curState->GetRotation().ToEulerAnglesXYZ(direction.x, direction.y, direction.z);
    
    // 根據 WASD 輸入計算位移
    RE::NiPoint3 moveVec = CalculateInputVector(); 
    
    // 套用座標
    RE::NiPoint3 currentPos = a_player->GetPosition();
    a_player->SetPosition(currentPos + (moveVec * g_FlightSpeed * a_deltaTime), true);
    
    // 禁用重力下墜
    a_player->GetCharRuntimeData().zVelocity = 0.0f;
}
```

---

## 步驟四：著陸與碰撞處理
1.  **地面偵測**:
    每秒進行 10 次向下射線檢測 (Raycast)。
    ```cpp
    if (RaycastDown(a_player) < 50.0f) {
        ExitFlightMode(a_player);
    }
    ```
2.  **碰撞處理**: 確保飛行時撞到牆壁會停下，這需要與 `CharacterProxy` 的膠囊體碰撞同步。

---

## 驗證方法
1.  **起飛測試**: 在高處跳下並按下按鍵，確認是否能懸停並播放飛行動畫。
2.  **轉向測試**: 旋轉鏡頭，確認前進方向是否隨之改變（3D 自由轉向）。
3.  **著陸測試**: 飛向地面，確認是否能平滑切換回走路動作，且不會穿透地板。

---
*文件路徑：architectures/classified/World/Tutorials/Tutorial_3D_Flight_Mod_Implementation.md*
