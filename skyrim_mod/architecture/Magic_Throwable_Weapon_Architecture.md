# Skyrim 可投擲武器架構 (Throwable Weapons Architecture)

在 Skyrim 引擎中，「投擲」並非原生的武器行為。要實現可投擲武器（如投擲斧、手榴彈或飛刀），通常需要結合 **物品數據**、**投影物 (Projectile)** 與 **腳本/C++ 邏輯**。

---

## 1. 核心組件：`RE::BGSProjectile`

無論使用哪種方案，**投影物**是靈魂。它決定了物體如何飛行、受到多少重力影響以及撞擊時發生什麼。

- **原始碼**: `include/RE/B/BGSProjectile.h`
- **關鍵屬性**:
    - **Type**: 必須設為 `Missile`。
    - **Gravity**: 決定下墜速度。設為 `1.0` 模擬真實重力。
    - **Speed**: 初始飛行速度。
    - **Impact Data Set**: 定義撞擊到金屬、木頭或肉體時的聲音與火花。
    - **Model (NIF)**: 投影物模型應與你手中拿的武器模型一致。

---

## 2. 實現方案對比

### 方案 A：偽裝成法術 (The Spell/Power Method)
這是最穩定的做法，類似於「投擲型火球術」。
1. **外觀**: 建立一個法術，其投影物模型設為「飛刀」。
2. **邏輯**: 玩家按下「施放」鍵，播放投擲動作，消耗一個清單中的「飛刀物品」，並發射該法術。
3. **優點**: 引擎原生支持，碰撞精確，且可附加爆炸效果。

### 方案 B：改裝彈藥系統 (The Ammo Method)
將飛刀定義為「箭矢（Ammo）」。
1. **做法**: 製作一個隱形的弓（或者是手的模型），並將飛刀設為彈藥。
2. **邏輯**: 射擊時，引擎自動處理消耗與飛行。
3. **優點**: 支援自動回收（如果設置了 `Can be Picked Up`）。

### 方案 C：C++ 動作鉤子 (The Advanced C++ Method)
透過 SKSE 監聽特定的攻擊動畫事件（Animation Event）。
1. **動作監聽**: 監聽 `WeaponSwing` 或自定義動作事件。
2. **物件生成**: 
    ```cpp
    // 虛擬代碼：手動發射投影物
    void ThrowWeapon(RE::Actor* a_thrower, RE::BGSProjectile* a_proj) {
        RE::Projectile::LaunchData data;
        data.origin = a_thrower->GetPosition();
        data.angles = a_thrower->GetAngle();
        data.projectile = a_proj;
        // ... 設置發射者與初始速度
        RE::Projectile::Launch(data);
    }
    ```
3. **優點**: 表現力最強，可以實現「扔出右手裝備的武器」的效果。

---

## 3. 技術挑戰與解決方案

### A. 投擲後的回收 (The Recovery)
如果玩家把斧頭扔出去了，他應該能撿回來。
- **做法**: 在投影物的 `Impact` 邏輯中，於撞擊位置生成（Spawn）一個對應的 `TESObjectREFR`（掉落物）。
- **關鍵類別**: `RE::TESDataHandler` 配合 `RE::TESObjectWEAP`。

### B. 旋轉動畫 (Rotation)
飛行的斧頭應該在空中旋轉。
- **解決方案**: 在投影物的 NIF 模型中加入一個特殊的 `NiControllerManager` 進行自旋動畫，或者在 C++ 中動態更新投影物的 `Rotation` 矩陣。

### C. 碰撞判定 (Collision)
- 如果是炸彈，需要結合 `RE::BGSExplosion`。
- 如果是飛刀，需要精確的 `Havok` 碰撞網格，否則會穿模。

---

## 4. 核心類別原始碼標註

- **`RE::BGSProjectile`**: `include/RE/B/BGSProjectile.h` - 飛行物理。
- **`RE::BGSExplosion`**: `include/RE/B/BGSExplosion.h` - 爆炸效果。
- **`RE::Projectile::LaunchData`**: 發射參數結構。
- **`RE::AnimationEvent`**: 用於同步投擲動作。

---
*文件路徑：architectures/classified/Magic/Throwable_Weapon_Architecture.md*
