# 步進式教學：實作 Monster Hunter 風格戰鬥機制 (Tutorial)

本教學將引導你實作 MH 風格戰鬥中的兩個關鍵技術：**打擊卡肉 (Hit-stop)** 與 **部位破壞檢測 (Part Breaking)**。

---

## 難度等級：專家 (Expert)

### 準備工具：
1. **CommonLibSSE-NG**: 核心開發環境。
2. **Open Animation Replacer (OAR)**: 用於替換連段動畫。
3. **NifSkope**: 用於查找魔物節點名稱。

---

## 步驟一：實作打擊卡肉 (Hit-stop)
當玩家擊中敵人時，產生瞬間的停頓感。

1.  **註冊事件監聽**: 監聽 `TESHitEvent`。
2.  **執行停頓邏輯**:
    ```cpp
    // 在 ProcessEvent 中
    if (a_event->cause->IsPlayerRef()) {
        auto victim = a_event->target->As<RE::Actor>();
        auto player = RE::PlayerCharacter::GetSingleton();
        
        // 1. 暫停動畫
        player->GetHostileAnimationGraph()->SetSpeed(0.05f);
        victim->GetHostileAnimationGraph()->SetSpeed(0.05f);
        
        // 2. 震動手柄/鏡頭
        RE::PlayerCamera::GetSingleton()->StartShake(0.5f, 0.1f);
        
        // 3. 恢復速度 (使用異步計時器)
        std::thread([=]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            player->GetHostileAnimationGraph()->SetSpeed(1.0f);
            victim->GetHostileAnimationGraph()->SetSpeed(1.0f);
        }).detach();
    }
    ```

---

## 步驟二：建立部位破壞系統
1.  **獲取命中節點**:
    在 `HandleHit` Hook 中，取得碰撞發生的 `NiAVObject`。
2.  **邏輯分佈**:
    - 如果節點名稱包含 `Tail`，將傷害累計至「尾部生命值」。
    - 當累計傷害 > 閾值時，執行：
        - `victim->PlayAnimation("SeverTail")`。
        - 在座標點 `DropItem("BrokenTail")`。

---

## 步驟三：體力與資源管理 (Stamina Management)
MH 的翻滾與大招消耗大量體力。
1.  **攔截動作輸入**: 
    監聽 `OnAnimationEvent`。當檢測到 `Dodge` 動畫開始時，強制扣除玩家 20 點體力。
2.  **懲罰機制**: 
    若體力歸零，禁止執行攻擊動畫事件，並播放 `Tired` 動作。

---

## 驗證方法
1.  **視覺驗證**: 觀察重型武器（如雙手錘）攻擊時是否有明顯的停頓感。
2.  **邏輯驗證**: 攻擊魔物頭部多次後，觀察其是否會觸發專屬的破壞硬直。
3.  **效能監控**: 確保 `Hit-stop` 的暫存線程不會造成內存洩漏或崩潰。

---
*文件路徑：architectures/classified/Systems/Tutorials/Tutorial_Monster_Hunter_Combat_Overhaul.md*
