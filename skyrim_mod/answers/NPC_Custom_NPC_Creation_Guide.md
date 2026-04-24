# Skyrim 自製特殊角色開發指南 (Custom NPC Creation)

在 Skyrim 中，一個「特殊角色」不僅僅是外觀不同，它是由 `RE::TESNPC` (FormType: 43) 定義的複雜個體，結合了視覺、AI 邏輯與社會關係。

---

## 1. 核心類別架構：`RE::TESNPC`

- **本質**: 一個「基礎對象 (Base Object)」。遊戲世界中看到的 NPC 實際上是 `RE::TESObjectREFR` 指向這個基礎對象。
- **原始碼**: `include/RE/T/TESNPC.h`

---

## 2. 關鍵組件 (Technical Components)

### A. AI Packages (行為包)
這是 NPC 的「靈魂」。它決定了角色在什麼時間去哪裡、做什麼。
- **種類**: 
    - `Find`: 去找某人或某物。
    - `Sandbox`: 在特定區域隨機互動（坐下、掃地、敲牆）。
    - `Travel`: 移動到特定座標。
    - `Procedure`: 複雜的動作序列。

### B. Factions (陣列與敵友關係)
決定了 NPC 會攻擊誰，或者誰會幫助 NPC。
- **Rank**: 在陣列中的等級。
- **Disposition**: 基礎好感度。

### C. Template (模板系統)
如果你想建立一群能力相似的守衛，可以使用 Template。
- **Inheritance**: 你可以選擇繼承特定的數據（如：屬性、法術、外觀、AI）。

---

## 3. 外觀數據：FaceGen

這是最容易出錯的地方。
- **NIF (Head Mesh)**: 位於 `meshes\actors\character\facegendata\facegeom\`。
- **DDS (Face Tint)**: 位於 `textures\actors\character\facegendata\facetint\`。
- **黑臉問題 (Dark Face Bug)**: 當 ESP 中的 NPC 數據與磁碟上的 FaceGen 檔案不匹配（FormID 改變或序號不對）時發生。

---

## 4. C++ 插件中的操作

```cpp
void SetNPCRelationship(RE::Actor* a_actor, int a_rank) {
    auto npc = a_actor->GetActorBase();
    if (npc) {
        // 修改 NPC 的數值
        npc->SetLevelMult(1.5f); // 設置等級倍率
        
        // 檢查是否為 Unique (唯一角色)
        if (npc->IsUnique()) {
            // 處理特殊劇情角色邏輯
        }
    }
}
```

---

## 5. 實作流程建議

1.  **在 Creation Kit (CK) 建立 NPC**: 設定 Race, Class, Stats。
2.  **配置 AI**: 給予至少一個 Sandbox Package，否則角色會原地發呆。
3.  **導出 FaceGen**: 在 CK 中選中 NPC 並按下 `Ctrl + F4`。
4.  **放置到世界**: 將 NPC 拖入某個 Cell，或透過腳本動態 `PlaceAtMe`。

---

## 6. 核心類別原始碼標註

- **`RE::TESNPC`**: `include/RE/T/TESNPC.h`
- **`RE::Package`**: `include/RE/T/TESPackage.h`
- **`RE::TESFaction`**: `include/RE/T/TESFaction.h`

---
*文件路徑：architectures/classified/NPC/Custom_NPC_Creation_Guide.md*
