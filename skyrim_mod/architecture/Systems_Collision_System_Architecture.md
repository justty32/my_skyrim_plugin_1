# Skyrim 碰撞系統架構：Havok 物理、碰撞層與射線偵測

Skyrim 的物理交互由 **Havok Physics** 引擎驅動。碰撞系統決定了你為何不能穿牆、武器如何擊中敵人，以及準星如何選中物體。

---

## 1. 碰撞的核心：`bhkCollisionObject`

在 NIF 模型中，碰撞數據並非直接使用高模 Mesh，而是掛載一個專門的物理對象。
- **原始碼**: `include/RE/B/bhkCollisionObject.h`
- **結構層級**:
    - **`bhkCollisionObject`**: 連接 3D 節點與物理世界的橋樑。
    - **`bhkRigidBody`**: 剛體。定義質量、摩擦力、旋轉阻力以及運動類型（Static/Dynamic）。
    - **`bhkShape`**: 碰撞幾何體。為了性能，通常使用簡化形狀：
        - **Box / Sphere / Capsule**: 運算最快。
        - **Compressed Mesh**: 複雜的地形或建築碰撞。

---

## 2. 碰撞層 (Collision Layers) 與過濾器

這是開發中最常遇到的部分。引擎透過「層」來決定哪些東西會互相碰撞。
- **原始碼**: `include/RE/M/MaterialIDs.h` (涉及 Layer 定義)
- **常用層級**:
    - **`L_STATIC`**: 建築、地形。NPC 和玩家會被阻擋。
    - **`L_ANIMSTATIC`**: 帶有動畫的靜態物體（如開關的門）。
    - **`L_BIPED`**: Actor（玩家和 NPC）的身體。
    - **`L_WEAPON`**: 揮動中的武器。
    - **`L_PROJECTILE`**: 飛行中的箭矢或火球。
    - **`L_TRIGGER`**: 觸發區域（不可見，但能偵測進入）。

---

## 3. 碰撞矩陣 (The Collision Matrix)
引擎維護著一張表，定義了「層 A 是否能碰撞層 B」。
- 例如：`L_PROJECTILE` 會與 `L_STATIC` 碰撞（箭射在牆上），但通常會穿過某些 `L_TRIGGER`。

---

## 4. 運行時交互：射線偵測 (Raycasting)

引擎如何知道你的準星對準了什麼？這就是 **Raycast**。
1.  從攝像機位置向前方發出一條「虛擬射線」。
2.  射線穿過物理世界，檢查與哪些 `bhkRigidBody` 相交。
3.  根據碰撞層過濾（例如：射線會忽略不可見的觸發區域）。
4.  返回距離最近的 `RE::TESObjectREFR`。

---

## 5. C++ 插件開發中的操作

### A. 修改碰撞層
你可以動態讓一個物體變得「不可觸碰」。
```cpp
void MakeGhost(RE::TESObjectREFR* a_ref) {
    auto collObj = a_ref->GetCollideObject();
    if (collObj) {
        auto rigidBody = collObj->GetRigidBody();
        // 將層級設為 NULL，物體將穿過一切
        rigidBody->SetCollisionLayer(RE::COL_LAYER::kNone);
        a_ref->UpdateCollisionFilter();
    }
}
```

### B. 手動射線偵測
```cpp
// 偽代碼：檢測前方是否有牆壁
RE::HavokPick hitInfo;
if (RE::HavokWorld::GetSingleton()->Raycast(startPos, endPos, hitInfo)) {
    auto hitRef = hitInfo.GetReference();
    // 獲取撞擊點信息
}
```

---

## 6. 技術總結
- **數據存儲**: 碰撞形狀儲存在 NIF 裡。
- **邏輯計算**: 由 Havok 引擎在背景線程或主循環物理階段計算。
- **更新關鍵**: 任何對物理屬性的修改，都必須調用 `UpdateCollisionFilter()` 才能在物理世界生效。

## 7. 核心類別原始碼標註
- **`RE::bhkWorld`**: `include/RE/B/bhkWorld.h` - 物理世界容器。
- **`RE::bhkRigidBody`**: `include/RE/B/bhkRigidBody.h` - 剛體屬性。
- **`RE::bhkShape`**: `include/RE/B/bhkShape.h` - 幾何形狀基類。
- **`RE::COL_LAYER`**: 碰撞層枚舉定義。
