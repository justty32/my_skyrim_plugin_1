# 步進式教學：實作自由操控的龍騎士系統 (Tutorial)

本教學將引導你完成從依附玩家到實作 3D 自由騎龍飛行的完整流程。

---

## 難度等級：極限 (Expert+)

### 準備工具：
1. **CommonLibSSE-NG**: 用於編寫接管邏輯。
2. **Behavior Data**: 確保龍的 `.hkx` 支援自由移動（通常需配合 `Dragon Combat Overhaul`）。

---

## 步驟一：玩家依附與動畫同步
當玩家對龍施放「服從龍吼」時：

```cpp
void MountDragon(RE::PlayerCharacter* a_player, RE::Actor* a_dragon) {
    // 1. 將玩家設為龍的載具
    a_player->SetVehicle(a_dragon);
    
    // 2. 鎖定玩家到龍的背部節點
    auto spineNode = a_dragon->Get3D()->GetObjectByName("NPC Spine2 [Spn2]");
    if (spineNode) {
        spineNode->AsNode()->AttachChild(a_player->Get3D(), true);
    }
    
    // 3. 通知行為圖進入騎乘模式
    a_player->NotifyAnimationGraph("DragonRideStart");
    a_dragon->NotifyAnimationGraph("DragonRideStart");
}
```

---

## 步驟二：自由飛行操控實作
攔截玩家輸入並轉換為 3D 空間移動：

1.  **更新每一幀**:
    獲取玩家鏡頭的 `Yaw` 與 `Pitch`。
2.  **位移計算**:
    ```cpp
    RE::NiPoint3 newPos = a_dragon->GetPosition();
    if (IsKeyPressed(W_KEY)) {
        // 向鏡頭指的方向飛行
        newPos += GetForwardVector(camera) * flightSpeed;
    }
    a_dragon->SetPosition(newPos, true);
    ```

---

## 步驟三：實作空中噴火 (C++ 實例)
將玩家的攻擊鍵對應到龍的噴火。

```cpp
void FireBreath(RE::Actor* a_dragon) {
    // 1. 播放龍的噴火動畫
    a_dragon->NotifyAnimationGraph("DragonFireStart");
    
    // 2. 獲取龍嘴座標
    auto mouth = a_dragon->Get3D()->GetObjectByName("MouthNode");
    
    // 3. 從龍嘴發射投影物 (Projectile)
    LaunchDragonFire(mouth->world.translate, GetAimDirection());
}
```

---

## 步驟四：著陸判定
1.  **監聽按鍵**: 當玩家按下 `Shift` 或 `Land` 鍵。
2.  **射線檢測**: 尋找下方最近的 `Navmesh`。
3.  **執行降落**: 強制龍執行 `Landing` 動畫，並在動畫結束後解除 `AttachChild`。

---

## 驗證方法
1.  **依附測試**: 確認玩家坐在龍背上的位置正確，且龍在轉彎時玩家不會「滑落」。
2.  **操控測試**: 轉動鏡頭向上看並前進，龍是否能真的垂直爬升。
3.  **戰鬥測試**: 確保在飛行中能準確擊中地面目標。

---
*文件路徑：architectures/classified/World/Tutorials/Tutorial_Dragon_Riding_System_Implementation.md*
