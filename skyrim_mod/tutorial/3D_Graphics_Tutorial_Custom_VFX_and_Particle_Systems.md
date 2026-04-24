# 實戰教學：自製特效與粒子系統 (VFX & Particle Systems)

本教學將引導你製作一個自定義的視覺特效，並將其應用到法術或武器上。

## 難度等級與準備工作
- **難度**: 高階 (Hard)
- **準備工具**:
    - **NifSkope**: 編輯 NIF 粒子發射器與著色器屬性。
    - **Blender**: 製作特效基礎網格。
    - **Photoshop/GIMP**: 製作特效貼圖 (DDS)。

---

## 實作步驟

### 步驟一：製作特效貼圖
1. 建立一個具備 Alpha 通道的 DDS 貼圖（如：火焰、星光）。
2. 建議使用 `BC7` 格式以獲得最佳品質與透明度。

### 步驟二：在 NifSkope 中設置特效節點
1. 打開一個特效 NIF（如 `MagicEffect.nif`）。
2. 找到 `BSEffectShaderProperty`。
3. 設置 `UV Animation`：讓貼圖在 X 或 Y 軸上持續位移，營造流動感。
4. 設置 `Emissive Color`：決定特效發光的顏色。

### 步驟三：配置粒子發射器 (BSParticleSystem)
1. 調整 `Emit Rate` (發射頻率) 與 `Lifetime` (粒子壽命)。
2. 使用 `Color Modifier` 設定粒子隨時間從亮轉暗。
3. 調整 `Velocity` (速度) 與 `Gravity` (重力) 模擬粒子噴發。

### 步驟四：將特效連結到遊戲物件
1. 在 CK 中建立一個 `Art Object` (BGSArtObject)，指向你的 NIF。
2. 在 `Magic Effect` 或 `Enchantment` 中引用這個 Art Object。
3. 當玩家施法或武器發光時，特效就會顯現。

---

## 代碼實踐 (C++ 動態附加特效)

```cpp
void ApplyVisualEffect(RE::Actor* a_target, const char* a_artEditorID) {
    auto dataHandler = RE::TESDataHandler::GetSingleton();
    auto artObj = dataHandler->LookupForm<RE::BGSArtObject>(a_artEditorID, "YourMod.esp");

    if (artObj && a_target) {
        // 將 ArtObject 附加到 NPC 身上
        a_target->ApplyArtObject(artObj);
        RE::ConsoleLog::GetSingleton()->Print("特效已應用於 %s", a_target->GetFullName());
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 施放帶有該特效的法術，觀察粒子是否按照預期軌跡移動。
- **問題 A**: 特效在白天看不見？
    - *解決*: 提高 `Emissive Multiple` (發光倍率) 或調整 `Alpha Blending` 模式。
- **問題 B**: FPS 嚴重下降？
    - *解決*: 減少 `BSParticleSystem` 的最大粒子數量 (`Max Count`)。過多的半透明面片會造成渲染壓力。
- **優化提示**: 善用 `uGridsToLoad` 原則，不要在遠處生成極其複雜的粒子特效。
