# 30. 技術實戰：天災降臨 (流星火雨與地面魔法陣)

本教學將教您如何製作一個具備震撼視覺效果的「禁咒」。核心包含兩大部分：地面展開的巨大發光魔法陣，以及從天而降的密集火球雨。

## 1. 核心邏輯
1.  **地面魔法陣 (Decal)**: 使用「貼花（Decal）」技術，將魔法陣貼圖投影到不平整的地形上，並設置發光屬性。
2.  **空間坐標計算**: 以玩家為中心，在周圍隨機選取天空中的發射點與地面的落點。
3.  **異步發射 (Meteor Shower)**: 透過計時器，在一段時間內持續發射多個「火球投射物」。

## 2. 代碼實現：流星火雨邏輯

```cpp
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

class MeteorRainManager {
public:
    static void CastMeteorRain(RE::Actor* a_caster) {
        if (!a_caster) return;
        auto pos = a_caster->GetPosition();

        // 1. 展開地面發光魔法陣 (Decal Projector)
        // 原始碼參考: include/RE/B/BGSDecalManager.h
        SpawnMagicCircle(pos);

        // 2. 啟動流星發射序列 (這裡使用簡單循環，實戰建議使用計時器分批發射)
        for (int i = 0; i < 20; i++) {
            LaunchSingleMeteor(pos);
        }
        
        RE::DebugNotification("末日已至！");
    }

private:
    static void SpawnMagicCircle(RE::NiPoint3 a_pos) {
        // 加載魔法陣貼花數據 (在 ESP 中定義一個 BGSTextureSet)
        auto magicCircleSet = RE::TESForm::LookupByID<RE::BGSTextureSet>(0xMY_MAGIC_CIRCLE_ID);
        
        // 使用 DecalManager 投影到地面
        // 這會讓貼圖完美貼合地形起伏，且不會懸空
        // RE::BGSDecalManager::GetSingleton()->SpawnDecal(..., magicCircleSet, ...);
        
        // 3D 細節：將該貼花的 ShaderProperty 設為 Additive 模式並賦予 Emissive 顏色，實現發光感
    }

    static void LaunchSingleMeteor(RE::NiPoint3 a_center) {
        // 1. 隨機計算天空中的起點 (高度 +2000)
        RE::NiPoint3 startPos = a_center;
        startPos.x += (rand() % 1000) - 500.0f;
        startPos.y += (rand() % 1000) - 500.0f;
        startPos.z += 2000.0f;

        // 2. 隨機計算地面落點
        RE::NiPoint3 endPos = a_center;
        endPos.x += (rand() % 1500) - 750.0f;
        endPos.y += (rand() % 1500) - 750.0f;

        // 3. 獲取火球投射物模板 (PlaceHolder: FireballProjectile)
        auto fireballBase = RE::TESForm::LookupByID<RE::BGSProjectile>(0x00015CFD);

        // 4. 發射投射物 (include/RE/P/Projectile.h)
        // 參數：發射者, 模板, 起點, 方向向量...
        // RE::Projectile::Launch(..., fireballBase, startPos, (endPos - startPos), ...);
    }
};
```

## 3. 3D 與特效技術細節解析

### A. 什麼是貼花 (Decals)？
普通的 3D 模型是平的，如果你在斜坡上生成一個圓形魔法陣模型，它會有一半埋入地下，一半浮在空中。
**貼花 (Decal)** 則是將貼圖「投影」到現有的幾何體上。它會根據地面的凹凸自動變形，就像一張緊貼地面的貼紙。

### B. 如何讓它發光？ (Emissive Multiplier)
在渲染架構中，魔法陣之所以看起來耀眼，是因為其 Shader 屬性：
-   **Emissive Color**: 設置為橙色或藍色。
-   **Emissive Multiple**: 設為大於 1.0 的值（如 5.0），這會讓它在 Bloom 濾鏡下產生強烈的光暈特效。

### C. 投射物的「天降」邏輯
在 Skyrim 中，投射物（Projectile）不僅可以從魔杖射出，也可以從空中虛擬座標發出。只要給予一個向下的 **速度向量 (Linear Velocity)**，它就會像流星一樣墜落。

## 4. 關鍵 API 標註
-   **`RE::BGSDecalManager`**: 負責處理血跡、燒痕和魔法陣投影。`include/RE/B/BGSDecalManager.h`
-   **`RE::BGSProjectile`**: 投射物數據定義。`include/RE/B/BGSProjectile.h`
-   **`RE::NiPoint3` 向量運算**: 計算發射軌跡。`include/RE/N/NiPoint3.h`

## 5. 擴展思路
-   **震動效果**: 每一枚火球落地時，調用 `RE::PlayerCharacter::GetSingleton()->ShakeCamera()` 增加打擊感。
-   **環境燃燒**: 在火球落點處 `PlaceAtMe` 一個微小的「噴火陷阱」，讓地面燃燒數秒。
