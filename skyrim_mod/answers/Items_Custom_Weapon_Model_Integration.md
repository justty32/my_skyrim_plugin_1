# Skyrim 自製武器模型整合指南 (Weapon Model Integration)

添加自製武器模型涉及 3D 建模、NIF 節點配置以及遊戲內數據表單 (Forms) 的設置。

---

## 1. 武器 NIF 檔案結構

一個正確的武器 NIF 檔案通常包含以下核心節點：

- **`BSFadeNode`**: 根節點，決定模型在遠處消失的邏輯。
- **`NiTriShape` / `BSTriShape`**: 實際的幾何網格數據。
- **`BSLightingShaderProperty`**: 
    - **Shader Type**: 必須設為 `Default` 或 `Weapon`。
    - **TextureSet**: 指向你的 Diffuse, Normal, Specular 貼圖。
- **物理與碰撞 (Havok)**:
    - **`bhkCollisionObject`**: 決定武器掉落在地上時的物理反應。
    - 如果沒有碰撞物件，武器掉落後會直接穿透地板掉入虛空。

---

## 2. 特殊節點標籤 (Marker Nodes)

為了讓引擎知道武器如何被手握持，NIF 中需要特定的節點名稱：
- **`Weapon`**: 主模型節點。
- **`WeaponMagicNode`**: 定義附魔發光效果的出發點。
- **`WeaponBloodNode`**: 決定砍人後血跡出現在武器的哪個區域。

---

## 3. 實作步驟

### A. 準備模型與貼圖
- **貼圖**: 必須是 `.dds` 格式，建議使用 BC7 壓縮。
- **模型**: 導出時需注意縮放比例（Skyrim 的單位約為 1 單位 = 1 厘米，但武器通常有特定的基準比例）。

### B. 在 ESP 中註冊 (Form Setup)
1.  **Static**: 先建立一個 Static Form，指向你的 NIF，用於在世界中擺放（非拾取狀態）。
2.  **Weapon (`TESObjectWEAP`)**: 
    - **Model**: 指向你的 NIF。
    - **First Person Model (`TESObjectSTAT`)**: 這是「第一人稱手部模型」。你可以使用相同的 NIF，或準備一個細節更高的版本。
    - **Stats**: 設置傷害、重量、價值。
    - **Keywords**: 設置如 `WeapTypeSword` 以獲得對應的技能樹加成。

### C. 處理碰撞與聲音
- **Impact Data Set**: 指定該武器砍到牆壁時發出的聲音。
- **Equip Slot**: 指定是右手、左手還是雙手。

---

## 4. 技術挑戰：對齊與動畫

- **對齊問題**: 武器在遊戲中「握不住」或「位置偏移」，通常是因為 NIF 的座標原點沒有對齊到握把位置。
- **動畫更新**: 如果你製作的是新型態武器（如：鐮刀，但使用劍的動作），可能需要修改 `Animation Event` 來修正砍擊判斷。

---

## 5. 核心類別原始碼標註

- **`RE::TESObjectWEAP`**: `include/RE/T/TESObjectWEAP.h`
- **`RE::NiNode`**: 底層 3D 節點基底。
- **`RE::NiTriShape`**: 幾何數據容器。

---
*文件路徑：architectures/classified/Items/Custom_Weapon_Model_Integration.md*
