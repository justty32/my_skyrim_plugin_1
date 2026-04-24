# Skyrim 新種族架構與開發指南 (New Race Architecture)

在 Skyrim 中，種族不僅是視覺上的差異，它是由 `RE::TESRace` 類別定義的一組複雜數據集合，涵蓋了從物理碰撞、骨架綁定到 AI 行為的所有基礎屬性。

---

## 1. 核心類別：`RE::TESRace`

- **原始碼**: `include/RE/T/TESRace.h`
- **本質**: 一個全局 Form (FormType: 19)，所有 NPC (`TESNPC`) 都必須引用一個 Race。

---

## 2. 關鍵數據結構 (Technical Components)

### A. Biped Data (身體構造與骨架)
這是決定種族是否能穿戴裝備的核心。
- **骨架路徑**: `data.skeletonModels`。男性與女性通常有不同的 NIF 路徑。
- **槽位定義 (Biped Object)**: 定義該種族具備哪些部分（如：頭、身、手、足、尾巴）。
- **縮放 (Scaling)**: `data.height` 和 `data.weight` 影響 NPC 的實際 3D 比例。

### B. Face Data (面部與頭部)
- **FaceGen**: 包含頭部模型、髮型、眼球模型以及表情變形數據。
- **Race Morph**: 決定了滑桿如何影響臉部特徵。

### C. Stats & Skills (數值與天賦)
- **初始數值**: 生命、魔法、耐力的基礎值與回覆率。
- **技能加成**: 每個種族通常有 6 個技能加成（如：諾德人 +10 雙手武器）。
- **特點 (Spells/Perks)**: 透過 `TESRace::spells` 列表添加種族主動或被動技能。

### D. Movement Data (移動屬性)
- 定義了走路、跑步、游泳與潛行時的速度與動畫頻率。
- 如果是四足種族（如：狼、龍），其路徑規劃與動畫邏輯與類人種族截然不同。

---

## 3. C++ 插件開發中的操作

如果你想在插件中判斷 NPC 是否屬於特定種族，或動態修改種族屬性：

```cpp
void CheckActorRace(RE::Actor* a_actor) {
    if (!a_actor) return;

    RE::TESRace* race = a_actor->GetRace();
    if (race) {
        // 檢查關鍵字 (Keywords)
        if (race->HasKeywordString("RaceDragon")) {
            // 處理龍類邏輯
        }

        // 獲取種族名稱
        const char* raceName = race->GetFullName();
    }
}
```

---

## 4. 製作新種族的實作步驟

### 第一步：定義 Biped 模型
你需要準備對應種族的 NIF 模型（身體、手、腳）。如果你的種族骨架與人類不同（例如：多了一對翅膀），你必須在 `TESRace` 中指定自定義的 `Skeleton.nif`。

### 第二步：設置護甲兼容性 (Armor Addon)
這是最繁瑣的一步。原版遊戲中的所有護甲（ArmorAddon）都標註了適用的種族。
- **問題**: 你的新種族預設無法穿戴任何原版護甲，穿上會變透明。
- **解決方案**: 
    1. 將新種族加入原版 `ArmorAddon` 的 `Compatible Races` 列表（通常使用插件如 `RaceCompatibility`）。
    2. 或將新種族設置為某個現有種族（如 `HumanRace`）的 **Inherit Data**。

### 第三步：配置行為與語音 (Voices & Behaviors)
- **Voice Type**: 指定該種族能使用的語音類型。
- **Attack Data**: 定義攻擊距離、角度與力道。

---

## 5. 技術挑戰：動態種族切換

如果你要開發一個「變身」模組（如：狼人、吸血鬼大君）：
1. **呼叫 `SetRace`**: `Actor::SetRace(RE::TESRace* a_race, bool a_isPlayer)`。
2. **3D 更新**: 切換種族後必須調用 `Update3DModel()`，否則模型會發生扭曲或崩潰。
3. **能力過渡**: 注意移除舊種族的 `Constant Effect` 法術，並添加新種族的。

---

## 6. 核心類別原始碼標註

- **`RE::TESRace`**: `include/RE/T/TESRace.h`
- **`RE::BIPED_MODEL`**: `include/RE/B/BipedObjects.h`
- **`RE::TESNPC`**: `include/RE/T/TESNPC.h` (種族的引用者)
