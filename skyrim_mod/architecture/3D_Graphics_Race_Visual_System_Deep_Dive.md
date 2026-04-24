# 深度解析：種族視覺系統 (Race Visuals & Skin)

`RE::TESRace` 是 Skyrim 外觀系統的總工程師。它決定了骨骼的選取、基礎模型以及最終呈現的貼圖。

---

## 1. 骨骼與身高 (Skeleton & Height)
- **原始碼**: `include/RE/T/TESRace.h`
- **機制**: 種族定義了基礎身高。例如：高精靈的身高係數大於 1.0，而木精靈則小於 1.0。
- **C++ 介入**: 你可以修改 `TESRace::data.height` 來讓遊戲中所有該種族的 NPC 統一變高。

---

## 2. 裸體與基礎 Mesh (Skin Data)
每個種族都定義了「裸體模型」。
- **組成**: 男性模型、女性模型。
- **Morphs**: 體重值（0-100）會觸發頂點插值，讓模型在「瘦」和「壯」之間變換。

---

## 3. 材質覆蓋 (Texture Overlays)
種族決定了皮膚的顏色和特徵。
- **Skin Texture**: 透過 `RE::BGSTextureSet` 指定。
- **化妝與臉部細節**: 透過 `RE::TESNPC::FaceData` 與種族預設的 FaceGen 邏輯疊加而成。

---

## 4. 特殊外觀：非人類種族 (Beast Races)
亞龍人與虎人使用了特殊的模型組件：
- **尾巴 (Tail)**: 作為獨立的 Biped 槽位掛載在骨骼上。
- **腳部骨骼**: 虎人的腳踝關節節點名稱與人類不同，這影響了他們走路的動畫。

---

## 5. C++ 實戰技巧：檢測與修改
```cpp
void MakeRaceTransparent(RE::TESRace* a_race) {
    // 透過修改種族模板，可以實現某些奇效
    // 例如：給種族添加一個隱形的魔效
}
```

---

## 6. 核心類別原始碼標註
- **`RE::TESRace`**: `include/RE/T/TESRace.h` - 視覺模板核心。
- **`RE::BGSTextureSet`**: `include/RE/B/BGSTextureSet.h` - 貼圖定義集。
- **`RE::BSTextureSet`**: `include/RE/B/BSTextureSet.h` - 運行時材質容器。
