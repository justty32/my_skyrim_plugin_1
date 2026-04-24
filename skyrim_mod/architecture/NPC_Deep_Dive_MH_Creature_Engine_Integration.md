# MH 魔物引擎整合深度解析 (Deep Dive)

要實作 Monster Hunter 級別的魔物互動，必須深入 Skyrim 的兩大核心黑盒：**Havok 物理系統** 與 **BSAnimationGraphManager**。

---

## 1. 精確部位判定：Havok 碰撞層

Skyrim 的原生碰撞通常是單一的大膠囊體。為了實現 MH 的部位判定，我們必須改造魔物的 `bhkRigidBody`。

### A. 多重碰撞體 (Multi-Part Colliders)
- **技術**: 在 NIF 中為每個主要骨骼（Head, Tail, WingL, WingR）建立獨立的 `bhkBoxShape` 或 `bhkSphereShape`。
- **碰撞過濾 (Collision Filtering)**: 將這些節點設置為特定的 `Collision Layer` (如 `L_BIPED_OBSTACLE`)。
- **C++ 偵測**: 
    在 `HandleHit` 事件中，透過 `RE::hkpCollidable` 獲取命中點所屬的 `Shape` 指針。透過比對 NIF 中的節點名稱，精確判斷是哪個部位受傷。

---

## 2. 行為圖變數與同步 (Graph Variables)

魔物的動作切換必須是瞬時且平滑的。

### A. 動態變數注入
我們不直接操作動畫，而是操作控制動畫的變數：
- **`fRageMeter`**: 浮點數，驅動紅光 Shader 的強度。
- **`iMoveSetIndex`**: 整數，決定下一個攻擊動作的選取池。
- **`bIsTired`**: 布林值，切換至疲勞動畫集。

### B. 動作同步 Hook
透過 Hook `RE::BSAnimationGraphManager::ProcessEvent`，我們可以攔截魔物發出的 `WeaponSwing` 標籤，並在該瞬間動態生成一個具備「強大擊退力」的物理衝量。

---

## 3. 大型魔物的導航與導向 (Navigation)

### A. 轉身融合 (Turn-In-Place Blending)
大型魔物不能像小型 NPC 一樣原地旋轉。
- **技術**: 使用 `Directional Blending`。行為圖根據玩家相對於魔物的角度，自動混合「向左轉」與「向右轉」的動畫步幅。

---

## 4. 核心類別原始碼標註

- **`RE::hkpWorld`**: 管理所有的碰撞物理。
- **`RE::BSAnimationGraphManager`**: 控制動畫狀態機的切換。
- **`RE::IAnimationGraphManagerHolder`**: 所有具備動畫的角色類別接口。

---
*文件路徑：architectures/classified/NPC/Architecture/Deep_Dive_MH_Creature_Engine_Integration.md*
