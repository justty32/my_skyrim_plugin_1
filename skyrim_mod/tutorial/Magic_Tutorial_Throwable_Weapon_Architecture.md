# 實戰教學：可投擲武器架構與實作 (Throwable Weapons)

本教學將引導你實作可投擲的斧頭或飛刀，包含飛行物理、碰撞傷害以及投擲後的物品回收。

## 難度等級與準備工作
- **難度**: 高階 (Hard)
- **準備工具**:
    - **Creation Kit (CK)**: 定義 `Projectile` (投影物)。
    - **CommonLibSSE-NG**: 處理投擲後的物件生成邏輯。

---

## 實作步驟

### 步驟一：建立投影物 (Projectile)
1. 在 CK 的 `SpecialEffect > Projectile` 建立新項目。
2. `Type` 設為 `Missile`。
3. 在 `Model` 指向你的飛刀或斧頭 NIF 檔案。
4. 設置 `Gravity` 為 1.0 (真實重力) 與 `Speed` (初速)。

### 步驟二：建立投擲法術
1. 建立一個 `Spell`，類型設為 `Power` (力量) 或 `Spell`。
2. 將其 `Projectile` 設置為你在第一步建立的項目。
3. 設置 `Explosion` (選用)：如果是手榴彈，可以添加爆炸效果。

### 步驟三：實作消耗機制 (C++)
1. 監聽施法事件。
2. 檢查玩家背包中是否有「飛刀物品」。
3. 若有，則允許施放並移除一個物品；若無，則攔截施法。

### 步驟四：實現投擲回收
1. 透過 C++ 監聽投影物的碰撞事件 (`RE::Projectile::OnImpact`)。
2. 在碰撞座標點（牆壁或地面）動態生成一個可拾取的物品引用 (`PlaceAtMe`)。

---

## 代碼實踐 (C++ 投影物發射範例)

```cpp
void FireThrowable(RE::Actor* a_thrower, RE::BGSProjectile* a_proj) {
    RE::Projectile::LaunchData data;
    data.projectile = a_proj;
    data.origin = a_thrower->GetPosition() + RE::NiPoint3(0, 0, 150); // 從手部位置發射
    data.angles = a_thrower->GetAngle();
    data.shooter = a_thrower;
    
    // 手動發射
    RE::Projectile::Launch(data);
}
```

---

## 常見問題與驗證
- **驗證方式**: 裝備投擲武器，對準牆壁射擊，確認武器是否插在牆上並可以重新拾取。
- **問題 A**: 武器飛行時不旋轉？
    - *解決*: 在 NIF 檔案中加入一個 `NiControllerManager` 的循環旋轉動畫，或在 C++ 中動態更新 `Rotation`。
- **問題 B**: 傷害太低？
    - *解決*: 投影物的傷害是在 `Magic Effect` 中定義的。確保法術威力與你的預期一致。
- **提示**: 結合 `uGridsToLoad` 原則，不要讓投影物的飛行距離超過加載範圍，否則會發生消失。
