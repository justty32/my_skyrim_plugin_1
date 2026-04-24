# Skyrim 新大地圖架構與開發指南 (New Worldspace Creation)

在 Skyrim 中，「大地圖」被稱為 **Worldspace**。它不是一個單一的房間，而是一個由無數個 Cell (單元) 網格組成的巨大開放空間。建立一個全新的大地圖（類似於《龍裔》DLC 中的索瑟姆島）是 Mod 開發中最浩大的工程之一。

---

## 1. 核心數據表單：`RE::TESWorldSpace`

- **原始碼**: `include/RE/T/TESWorldSpace.h`
- **本質**: 一個全局管理器，負責統籌該區域的天氣、地形高度、水面高度以及地圖介面。

---

## 2. 建立新 Worldspace 的五大核心步驟

### 第一步：定義 Worldspace (在 Creation Kit 中)
1. **建立 Form**: 在 CK 中建立一個新的 `Worldspace` 對象。
2. **設置氣候 (Climate)**: 選擇該世界的氣候類型（決定了會下雪、下雨還是晴天）。
3. **水面高度 (Water Level)**: 設定全局海平面高度。所有低於此 Z 軸座標的地形都會被水覆蓋。
4. **Parent Worldspace**: 如果你的地圖依附於天際省（如一個開放式的大城），可以設定繼承關係；如果是完全獨立的島嶼，則設為 None。

### 第二步：生成地形 (Terrain & Heightmap)
大地圖的基底不是 3D 模型，而是高度圖 (Heightmap)。
1. **匯入高度圖**: 你可以使用外部工具（如 World Machine）生成灰階的 RAW 格式圖片，並匯入 CK 轉換為地形的高低起伏。
2. **繪製地表 (Landscaping)**: 在 CK 中為不同的地形刷上不同的貼圖（如：草地、雪地、泥土）。這會自動生成對應的物理摩擦力與腳步聲。

### 第三步：生成遠景 (LOD - Level of Detail)
如果不生成 LOD，玩家在地圖中只能看到周圍 5x5 的單元，遠處會是虛空。
1. **Terrain LOD**: 將遠處的高低地形轉換為低面數的網格。
2. **Object LOD**: 為遠處的建築、樹木生成低面數的替代模型（通常使用 `xLODGen` 或 `DynDOLOD` 工具）。
3. **Map LOD**: 生成玩家按下 `M` 鍵時看到的 3D 大地圖。

### 第四步：導航網格 (Navmesh)
- **挑戰**: 大地圖的 Navmesh 是切分為無數個 Cell 的。
- **邊界連結**: 相鄰單元之間的 Navmesh 頂點必須完美縫合，否則 NPC 在跨越單元邊界時會卡住。這通常需要使用 CK 的 `Finalize Navmesh` 功能。

### 第五步：建立傳送門 (Transition)
玩家如何前往你的新世界？
1. **載入室內**: 通常會在天際省建立一個洞穴或船隻（Interior Cell）。
2. **Teleport Door**: 在室內放置一扇門，將其「Teleport」目標連結到你新 Worldspace 的某個預設出生點 (Marker)。

---

## 3. C++ 插件中的操作與交互

在 SKSE 插件中，你可以動態檢測玩家是否身處你的新地圖，並執行特殊邏輯（例如：在你的地圖中禁用快速旅行）。

```cpp
void CheckPlayerWorldspace() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto currentWorld = player->GetWorldspace();
    
    if (currentWorld) {
        // 比對 EditorID 或 FormID
        if (strcmp(currentWorld->GetFormEditorID(), "MyCustomWorldspace") == 0) {
            // 玩家進入了新地圖
            // 執行特殊邏輯，如：動態改變全局重力
        }
    }
}
```

---

## 4. 技術與效能陷阱

- **Cell 數量限制**: 雖然引擎支援龐大的座標範圍，但在邊緣座標極端大（如 X/Y 超過 64）的地方，Havok 物理引擎會因為浮點數精度問題而產生「抖動（Havok Bug）」。
- **Occlusion Culling (遮擋剔除)**: 在山谷或城市中，必須放置 `Occlusion Plane`，否則引擎會渲染山背後玩家看不見的所有物體，導致嚴重的效能下降。

---

## 5. 核心類別原始碼標註

- **`RE::TESWorldSpace`**: `include/RE/T/TESWorldSpace.h` - 管理器。
- **`RE::TESClimate`**: 氣候數據。
- **`RE::TESWaterForm`**: 水體屬性。

---
*文件路徑：architectures/classified/World/New_Worldspace_Creation_Guide.md*
