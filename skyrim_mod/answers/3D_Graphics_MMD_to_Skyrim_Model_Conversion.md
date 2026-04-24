# MMD 模型導入 Skyrim 技術解答 (MMD to Skyrim Model Conversion)

將外部 MMD (MikuMikuDance) 模型導入 Skyrim 並賦予 NPC 使用，是一項涉及跨平台模型格式轉換與骨架重定性 (Retargeting) 的進階 3D 技術任務。

---

## 1. 核心架構差異

### A. 格式與拓撲
- **MMD**: 使用 `.pmx` 或 `.pmd` 格式。通常具備極高面數與專為動漫渲染設計的材質。
- **Skyrim**: 使用 `.nif` (NetImmerse Format) 格式。頂點數受到引擎效能限制，且必須包含完整的 Havok 碰撞數據與頂點色資訊。

### B. 骨架系統 (The Skeleton Problem)
這是轉換中最困難的部分。
- **MMD 骨架**: 命名規則為日文（如 `全ての親`, `足首`），且層級結構與西方引擎不同。
- **Skyrim 骨架 (`Skeleton.nif`)**: 命名規則嚴格（如 `NPC Root [Root]`, `NPC L Foot [Lft ]`）。
- **衝突**: Skyrim 的動畫是綁定在特定骨骼名稱上的。如果你直接導入 MMD 骨架，NPC 將無法播放任何動作，會呈現 T-Pose。

---

## 2. 技術解決方案：骨架重新綁定 (Rigging & Weighting)

要讓 MMD 模型在遊戲中動起來，你必須將模型的「頂點權重」從 MMD 骨架轉移到 Skyrim 的原生骨架上。

1.  **骨骼匹配 (Bone Mapping)**: 建立一個對應表，將 MMD 的「足首」映射到 Skyrim 的 `L Foot`。
2.  **頂點權重轉移 (Weight Transfer)**: 讓模型表面跟著 Skyrim 的骨骼移動。如果轉移不精確，角色在走路時關節處會發生嚴重的拉伸或扭曲。

---

## 3. 材質與著色器轉換

MMD 通常使用內嵌貼圖或簡單的卡通渲染，而 Skyrim 需要：
- **DDS 格式**: 所有的貼圖必須轉換為帶有 Mipmaps 的 `.dds`。
- **法線貼圖 (Normal Map)**: MMD 原生不包含法線貼圖（_n.dds），你可能需要手動生成以產生 3D 質感。
- **環境遮擋 (AO)**: 為了讓角色在遊戲環境中不顯得「浮空」，需要正確的陰影屬性。

---

## 4. 實作路徑：如何讓 NPC 穿上它？

你可以透過兩種方式將模型賦予 NPC：

### 方案 A：替換 NPC 基礎模型 (Base Model Replace)
- **做法**: 直接將轉換後的 NIF 設為 NPC 的 `Race` 模型或 `FaceGen` 模型。
- **優點**: 一勞永逸。
- **缺點**: 可能導致原有的護甲系統失效（因為 MMD 模型通常是一體成型的）。

### 方案 B：作為護甲物件 (Armor/Outfit)
- **做法**: 將模型拆分為「頭部」、「身體」等部件，並製作成 `Armor` (護甲) 物件，讓 NPC 穿上。
- **優點**: 兼容性強，可以隨時更換。

---

## 5. 核心類別與資源

- **`RE::TESNPC`**: 指定模型路徑。
- **`RE::TESObjectARMA`**: 定義護甲與種族的模型連結。
- **`RE::NiNode`**: 3D 數據的容器。

---
*文件路徑：architectures/classified/3D_Graphics/Answers/MMD_to_Skyrim_Model_Conversion.md*
