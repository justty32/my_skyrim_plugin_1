# Skyrim 槍械系統實作架構 (Firearm System Architecture)

要在 Skyrim 中實作一套具備「彈匣 (Magazine)」與「換彈 (Reloading)」機制的現代槍械系統，必須跳脫原有的「弓箭/弩箭」逻辑，並結合 SKSE C++ 插件來處理數據與邏輯同步。

---

## 1. 核心架構：數據模型

### A. 武器基底 (The Weapon)
- **類別**: `RE::TESObjectWEAP`
- **建議類型**: 設為 **弩 (Crossbow)**。
- **原因**: 弩原生具備「填裝後發射」的動作循環，比弓箭更適合模擬半自動或栓式步槍。

### B. 彈匣管理 (Magazine Logic)
Skyrim 的原生物品欄無法記錄單一武器的「剩餘彈頭數」。
- **技術方案**: 
    1. **ExtraData 儲存**: 使用 `RE::ExtraDataList` 為每個武器實例 (REFR) 附加一個 `ExtraInt` 來紀錄當前彈藥。
    2. **C++ 映射表**: 在插件中維護一個 `std::map<RE::ObjectRefHandle, int>`。
    3. **邏輯**: 每次射擊（觸發 `WeaponSwing` 事件）時，將該數值 `-1`。

---

## 2. 換彈機制 (Reloading)

這涉及動畫與數據的深度耦合。

### A. 動畫觸發
1. **輸入監聽**: 透過 SKSE 監聽 `R` 鍵（或自定義按鍵）。
2. **動作播放**:
   ```cpp
   // 發送換彈動畫事件
   a_actor->NotifyAnimationGraph("ReloadStart");
   ```

### B. 數據同步 (Animation Events)
不要在按下按鍵時立即補滿子彈。應在動畫的特定幀（如手塞入彈匣的瞬間）進行：
- **Hook 事件**: 監聽動畫圖發出的標籤事件（如 `ReloadComplete` 或 `SoundPlay.ReloadDraw`）。
- **處理函數**: 
    - 檢查玩家清單中是否有足夠的備用彈藥。
    - 扣除備用彈藥，並更新武器的 `ExtraData`（彈匣數）。

---

## 3. 彈道系統 (Ballistics)

### 方案 A：物理投影物 (Projectile)
- **原理**: 使用 `RE::BGSProjectile`。
- **特點**: 有飛行速度、下墜、飛行時間。
- **適用**: 狙擊槍、榴彈砲。

### 方案 B：瞬時命中 (Hitscan)
- **原理**: 射擊瞬間進行射線檢測 (Raycast)。
- **實作**:
  ```cpp
  // 使用 Havok 射線檢測
  RE::Projectile::LaunchData data;
  // ... 設置發射路徑
  // 透過 C++ Hook 立即計算碰撞點並套用傷害
  ```
- **適用**: 手槍、突擊步槍、近戰。

---

## 4. UI 與 HUD 整合

玩家需要看到剩餘子彈數。

- **自定義選單**: 建立一個基於 `RE::IMenu` 的 Flash (.swf) 視圖。
- **數據推播**: 
    - 在每一幀（`Main::Update`）或數據變更時，透過 `RE::GFXMovieView::SetVariable` 將彈藥數傳送給 UI。
    - **範例**: `_root.AmmoCount.text = currentMagSize.toString();`

---

## 5. 技術挑戰與優化

1.  **連發射擊 (Auto-Fire)**:
    - 需要自定義動畫節點，讓動畫循環播放而不重啟。
    - 在 C++ 中使用計時器控制射擊間隔。
2.  **彈殼拋出 (Shell Eject)**:
    - 射擊時，在武器的 `Eject` 骨骼節點座標生成一個小的 3D 物件，並給予一個隨機的物理衝量。
3.  **後座力 (Recoil)**:
    - 動態修改 `RE::PlayerCharacter` 的視角矩陣（Camera Angle）。

---

## 6. 核心類別原始碼標註

- **`RE::TESObjectWEAP`**: `include/RE/T/TESObjectWEAP.h`
- **`RE::ExtraDataList`**: `include/RE/E/ExtraDataList.h`
- **`RE::GFXMovieView`**: `include/RE/G/GFXMovieView.h` (UI 核心)
- **`RE::Projectile`**: `include/RE/P/Projectile.h`

---
*文件路徑：architectures/classified/Items/Firearm_System_Implementation.md*
