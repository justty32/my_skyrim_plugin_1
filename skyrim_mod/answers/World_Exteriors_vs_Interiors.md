# Skyrim 引擎解析：室外 (Exteriors) vs 室內 (Interiors)

在 Skyrim 引擎中，室外與室內不僅僅是視覺上的區別，它們在數據結構、加載機制以及物理邏輯上都有著截然不同的運作方式。

---

## 1. 數據容器的本質

### A. 室內 (Interiors)
- **類別**: 獨立的 `RE::TESObjectCELL`。
- **原始碼**: `include/RE/T/TESObjectCELL.h`
- **特徵**: 它是「有限的、封閉的」空間。每個室內單元都有自己的照明設置（Lighting Template）和聲學屬性。
- **座標系統**: 以單元中心的 `(0, 0, 0)` 為絕對基準。

### B. 室外 (Exteriors)
- **類別**: 屬於某個 `RE::TESWorldSpace` 的格子。
- **原始碼**: `include/RE/T/TESWorldSpace.h`
- **特徵**: 它是「無限的、網格化的」。整個天際省（Tamriel）是一個巨大的 WorldSpace，被劃分為無數個 $64 \times 64$ 單元的網格。
- **座標系統**: 使用全局座標（Global Coordinates），由單元網格座標 `(X, Y)` 和單元內相對位置組成。

---

## 2. 關鍵系統對比

| 特性 | 室外 (Exteriors) | 室內 (Interiors) |
| :--- | :--- | :--- |
| **加載機制** | 動態加載。當玩家移動時，引擎會異步加載鄰近的網格。 | 瞬間加載。透過“加載門”切換，舊單元通常被完全卸載。 |
| **照明 (Lighting)** | 受天氣系統 (`TESWeather`) 控制。包含晝夜循環、太陽位置。 | 受單元模板控制。光照通常是恆定的（除非腳本修改）。 |
| **導航 (Navmesh)** | 巨大的連續網格。存在跨單元（Cell Boundary）鏈接。 | 孤立的網格。不與外部世界直接連通，只能透過傳送門交互。 |
| **物理 (Havok)** | 需要處理地形高度圖 (Heightmap)。 | 主要是靜態物件 (Static) 與地板的碰撞。 |
| **遠景 (LOD)** | 擁有 LOD 數據，可以在遠處看到低模物體。 | 無 LOD。超出裁剪平面（Clipping Plane）的物體直接不渲染。 |

---

## 3. C++ 插件開發中的判斷

如果你想知道玩家當前在哪種環境，代碼如下：

```cpp
auto player = RE::PlayerCharacter::GetSingleton();
auto currentCell = player->GetParentCell();

if (currentCell) {
    if (currentCell->IsInterior()) {
        // 處理室內邏輯 (例如：自動收起武器)
    } else {
        // 處理室外邏輯 (例如：動態改變視距)
    }
}
```

---

## 4. 進階區別：時間流逝與行為

- **Wait (等待)**: 在室外等待時，引擎會模擬陰影的移動；在室內等待，環境光通常保持不變。
- **AI 旅行**: 
    - NPC 在室外旅行時，引擎會使用簡化的坐標插值。
    - NPC 在室內時，通常會嚴格執行 Sandboxing 包中的互動動作（如坐下、掃地）。

---

## 5. 總結：開發者的視角

- **建城 (教學 21)**：如果你在**室外**建城，你必須考慮 LOD 和 Navmesh 邊界問題。
- **隨機地牢 (教學 04)**：在**室內**生成空間要容易得多，因為你不需要擔心背景地貌和天氣的干擾。

## 6. 核心類別原始碼標註

- **`RE::TESObjectCELL`**: `include/RE/T/TESObjectCELL.h` - 空間載體。
- **`RE::TESWorldSpace`**: `include/RE/T/TESWorldSpace.h` - 大世界管理器。
- **`RE::Sky`**: `include/RE/S/Sky.h` - 室外環境（天空、太陽、大氣）的核心單例。
