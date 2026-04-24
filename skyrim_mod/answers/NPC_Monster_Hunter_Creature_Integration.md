# Monster Hunter 魔物導入技術架構 (Creature Integration)

要將 Monster Hunter 中的大型魔物（如雄火龍 Rathalos）導入 Skyrim，不僅僅是替換模型，而是要建立一套包含自定義骨架、獨立 AI 邏輯與物理判定系統的完整生態。

---

## 1. 核心架構：全新的 `RE::TESRace`

魔物必須擁有獨立的種族定義，而非修改原版的龍或巨人。

### A. 自定義骨架 (The Custom Skeleton)
- **挑戰**: MH 魔物的骨骼數量與層級遠超 Skyrim 原生生物。
- **方案**: 建立 `Monster_Skeleton.nif`。
    - **Node 命名**: 必須包含關鍵的碰撞節點（如 `NPC Head`, `NPC Tail1`~`TailN`）以支援部位破壞。
    - **翼膜與細節**: 使用 `NiControllerManager` 管理次要骨骼（如翅膀摺疊）的自動化。

### B. 行為圖 (Behavior Graph / Havok)
- **本質**: 這是魔物的「大腦」。
- **實作**: 必須為魔物撰寫專屬的 `.hkx` 行為文件。
    - **狀態機**: 定義 `Idle` -> `Threaten` -> `Attack` -> `Recovery` 的轉換邏輯。
    - **動態轉向**: 實作魔物跟隨玩家位置動態旋轉（Turn in place）的動畫融合。

---

## 2. 戰鬥 AI 與環境互動

### A. 5x5 網格中的 AI 活性
- **技術限制**: 當魔物處於玩家 5x5 加載網格邊緣時，AI 精度會下降。
- **優化**: 為大型魔物強制開啟 `Persistent` 標籤，確保其位置與狀態在遠距離時仍能由 C++ 插件精確計算。

### B. 招式選擇邏輯 (Move Selection)
原版的 `CombatStyle` 太過簡單。我們在 C++ 中實作進階邏輯：
- **距離判定**: 玩家在遠處時觸發「龍息」或「衝鋒」。
- **側位判定**: 玩家在側面時觸發「鐵山靠」或「甩尾」。

---

## 3. 整合部位破壞 (Connection to Part Breaking)

與我們先前建立的戰鬥系統連動：
1. **模型替換 (NIF Swapping)**: 當「尾部生命值」歸零，調用 `RE::NiNode::DetachChild()` 移除完整的尾部，並 `AttachChild()` 一個斷尾斷面的模型。
2. **掉落物生成**: 在斷裂座標點生成一個可採集的 `RE::TESObjectREFR`（魔物素材）。

---

## 4. 核心類別原始碼標註

- **`RE::TESRace`**: `include/RE/T/TESRace.h` - 種族定義。
- **`RE::CombatStyle`**: `include/RE/C/CombatStyle.h` - 基礎戰鬥傾向。
- **`RE::CharacterProxy`**: 處理大型魔物的複雜物理碰撞。
- **`RE::BGSScene`**: 用於實作魔物登場的「開場動畫」劇情。

---
*文件路徑：architectures/classified/NPC/Answers/Monster_Hunter_Creature_Integration.md*
