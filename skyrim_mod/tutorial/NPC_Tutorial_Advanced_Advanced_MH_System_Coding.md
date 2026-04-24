# 教學：深度實作魔物招式判定與代謝系統 (Tutorial)

本教學將帶領你寫入一個能夠精確控制魔物攻擊「判定幀」與「疲勞狀態」的 SKSE 插件。

---

## 難度等級：極限 (Expert+)

### 準備工具：
1. **Visual Studio 2022**: 配置 CommonLibSSE-NG。
2. **NifSkope**: 用於檢查 `NiCollisionObject` 的層級。

---

## 步驟一：精確判定框 (Hitbox) 控制
原版的判定框是持續存在的。MH 要求只有在「揮動瞬間」才有判定。

1.  **監聽動畫事件**:
    ```cpp
    void OnAnimationEvent(RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source, const RE::BSAnimationGraphEvent* a_event) {
        if (a_event->tag == "MHA_HitboxOn") {
            EnableCollision(a_event->holder, "WeaponNode");
        } else if (a_event->tag == "MHA_HitboxOff") {
            DisableCollision(a_event->holder, "WeaponNode");
        }
    }
    ```

---

## 步驟二：疲勞代謝系統
魔物的動作會消耗能量，能量低時進入疲勞。

1.  **建立代謝計時器**: 
    在 `Main::Update` 中，每秒根據魔物的動作（衝鋒、大招）扣除其 `fStamina`。
2.  **強制動畫切換**:
    ```cpp
    if (monsterData.stamina <= 0) {
        // 發送事件讓行為圖切換到疲勞狀態
        monster->NotifyAnimationGraph("ForceExhausted");
        // 同時大幅降低轉身速度
        monster->SetActorValue(RE::ActorValue::kSpeedMult, 50.0f);
    }
    ```

---

## 步驟三：部位破壞特效同步
當部位生命值歸零時，觸發瞬間的碎裂效果。

1.  **播放特效與聲音**:
    使用 `RE::BGSExplosion` 模擬碎裂的衝擊波。
2.  **更新行為圖變數**:
    `monster->SetGraphVariableBool("bTailSevered", true)`，這會讓魔物的後續攻擊（如甩尾）的攻擊範圍判定自動縮減。

---

## 驗證與調優
1.  **判定框可視化**: 使用開發插件顯示物理碰撞體，確保其與動畫完全同步。
2.  **存檔測試**: 砍斷尾巴後存檔並重讀，確認尾巴依然是斷裂狀態。
3.  **效能監控**: 確保 `OnAnimationEvent` 的處理不會導致在高頻攻擊時掉幀。

---
*文件路徑：architectures/classified/NPC/Tutorials/Tutorial_Advanced_MH_System_Coding.md*
