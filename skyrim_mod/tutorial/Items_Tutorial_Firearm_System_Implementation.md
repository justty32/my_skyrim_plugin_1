# 實戰教學：槍械系統實作與換彈機制 (Firearm System)

本教學將引導你實作一套具備「彈藥計數」與「換彈動畫」的現代槍械系統。

## 難度等級與準備工作
- **難度**: 極高 (Very Hard)
- **準備工具**:
    - **CommonLibSSE-NG**: 處理核心邏輯。
    - **Open Animation Replacer (OAR)**: 管理換彈動畫。
    - **Creation Kit (CK)**: 建立武器與彈藥。

---

## 實作步驟

### 步驟一：武器基底設置
1. 在 CK 中建立一個 `Weapon`，類型設為 `Crossbow` (弩)。
2. 準備槍械模型 NIF，確保其節點與弩的骨架兼容。

### 步驟二：實作彈匣計數 (C++)
1. 使用 `std::map<RE::ObjectRefHandle, int>` 紀錄每把槍的剩餘子彈。
2. Hook `RE::Projectile::Launch` 或監聽射擊動畫事件。
3. 每次射擊時，將對應槍枝的子彈數 `-1`。當歸零時，強制攔截後續射擊。

### 步驟三：換彈動畫與邏輯
1. 監聽 `R` 鍵。
2. 呼叫 `NotifyAnimationGraph("ReloadStart")` 播放換彈動作。
3. 監聽動畫事件標籤 `ReloadComplete`。
4. 在回調中，從玩家背包扣除彈藥物品，並重置彈匣計數。

### 步驟四：槍械 HUD 顯示
1. 製作一個簡單的 Flash UI (參考 `Custom_UI` 教學)。
2. 每當子彈數變更時，將數值推送到 UI 顯示。

---

## 代碼實踐 (C++ 換彈邏輯範例)

```cpp
void HandleReload(RE::Actor* a_actor) {
    if (!a_actor->IsPlayer()) return;

    // 檢查是否有備用彈藥
    auto ammo = RE::TESForm::LookupByID<RE::TESBoundObject>(AMMO_FORM_ID);
    if (a_actor->GetItemCount(ammo) > 0) {
        // 播放換彈動畫
        a_actor->NotifyAnimationGraph("ReloadStart");
        RE::DebugNotification("正在換彈...");
    } else {
        RE::DebugNotification("彈藥不足！");
    }
}

// 監聽動畫事件重置計數
void OnAnimationEvent(RE::BSTSmartPointer<RE::BSAnimationGraphEvent>& a_event) {
    if (a_event->tag == "ReloadComplete") {
        g_currentMag = 30; // 假設彈匣容量為 30
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 射擊 30 次後，確認是否無法再射擊，按下 R 鍵後是否能重新射擊。
- **問題 A**: 射擊速度太快導致動畫崩潰？
    - *解決*: 在 C++ 中設置射擊間隔計時器 (Cooldown)。
- **問題 B**: 換彈時子彈沒扣除？
    - *解決*: 確保 `ReloadComplete` 事件已在動畫圖 (.hkx) 中標註，否則 C++ 接收不到信號。
- **優化提示**: 為不同的槍械設置不同的 `Recoil` (後座力) 參數，動態修改相機角度。
