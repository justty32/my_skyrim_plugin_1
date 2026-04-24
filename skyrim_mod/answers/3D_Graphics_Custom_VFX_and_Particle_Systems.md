# Skyrim 自製特效與粒子系統 (VFX & Particle Systems)

在 Skyrim 中，視覺特效 (VFX) 主要是透過 NIF 檔案中的特殊節點與著色器屬性，配合貼圖與粒子發射器來實現的。

---

## 1. 核心組成技術

### A. 著色器屬性 (Shader Properties)
- **`BSEffectShaderProperty`**: 專門用於特效。
- **關鍵功能**: 
    - **UV 動畫 (Texture Scrolling)**: 讓貼圖在模型表面持續滾動，模擬流動的岩漿或能量。
    - **發光 (Emissive)**: 即使在黑暗中也能發光的顏色與強度。
    - **透明度混合 (Alpha Blending)**: 控制特效如何與背景融合。

### B. 粒子系統 (Particle Systems)
- **類別**: `BSParticleSystem`
- **工作原理**: 發射大量的微小面片，並根據控制器（Controllers）決定它們的生命週期、速度、顏色變化與縮放。
- **關鍵控制器**: 
    - `BSPSysSimpleColorModifier`: 控制粒子從出生到死亡的顏色漸變。
    - `BSPSysGravityModifier`: 模擬重力。

---

## 2. 實作流程

### 第一步：NIF 模型製作
1.  **使用工具**: Blender + NifSkope。
2.  **模型層級**: 特效通常不需要複雜的幾何體，往往只是一個簡單的平面（Plane）或球體，重點在於 `BSEffectShaderProperty`。
3.  **材質路徑**: 指向自定義的 DDS 貼圖（通常包含 Alpha 通道）。

### 第二步：定義觸發器 (Trigger)
特效必須依附於某個遊戲物件才能顯示：
- **Art Object (`BGSArtObject`)**: 用於法術施放時手部的光球。
- **Effect Shader (`TESEffectShader`)**: 直接覆蓋在 NPC 身體上的效果（如：被火燒時全身冒煙）。
- **Explosion (`BGSExplosion`)**: 炸彈爆炸瞬間產生的粒子與火光。

### 第三步：C++ / 腳本控制
```cpp
// 在特定座標生成特效
void SpawnEffect(RE::BGSArtObject* a_art, RE::Actor* a_target) {
    if (a_art && a_target) {
        a_target->ApplyArtObject(a_art); // 讓 NPC 身上出現該特效
    }
}
```

---

## 3. 技術要點與優化

- **Overdraw (過度渲染)**: 畫面中出現過多重疊的半透明粒子會導致 FPS 驟降。
- **動態光源**: 部分特效（如火球）會帶有 `NiPointLight`，能照亮周圍環境，但每個場景的光源數量有限，過多會造成閃爍。
- **物理碰撞**: 粒子預設沒有碰撞。若要實現「碰到地面後彈起」的效果，需加入 `BSPSysCollisionModifier`。

---

## 4. 核心類別原始碼標註

- **`RE::BGSArtObject`**: `include/RE/B/BGSArtObject.h`
- **`RE::TESEffectShader`**: `include/RE/T/TESEffectShader.h`
- **`RE::NiParticleSystem`**: 底層渲染類別。

---
*文件路徑：architectures/classified/3D_Graphics/Custom_VFX_and_Particle_Systems.md*
