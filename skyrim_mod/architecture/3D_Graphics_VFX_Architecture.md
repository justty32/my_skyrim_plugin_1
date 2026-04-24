# Skyrim 特效架構：粒子系統、視覺效果與動態觸發

在 Skyrim 中，「特效」並非單一文件，而是一個結合了數據定義（ESP）、3D 粒子模型（NIF）與渲染指令的複合系統。

---

## 1. 特效的組成部分

### A. 數據定義：`BGSVisualEffect` (ESP)
- **作用**: 它是特效的「身份證」，定義了特效的持續時間、優先級以及關聯的模型路徑。
- **原始碼**: `include/RE/B/BGSVisualEffect.h`

### B. 視覺載體：粒子系統 (NIF)
- **內容**: 特效的核心通常是包含 `NiParticleSystem` 塊的 NIF 文件。
- **組成**:
    - **Emitter (發射器)**: 定義粒子產生的位置。
    - **Particles (粒子)**: 小的幾何面片，貼有透明度高的貼圖。
    - **Modifiers (修改器)**: 控制粒子的重力、旋轉、顏色隨時間的變化。

### C. 渲染層：Shader & Alpha
- 特效大量使用 **Additive Blending (加法混合)**，讓光效看起來呈現出透亮的發光感。

---

## 2. 資源所在與路徑

- **模型**: `Data\Meshes\Effects\` (多為 `.nif`)。
- **貼圖**: `Data\Textures\Effects\` (多為帶有 Alpha 通道的 `.dds`)。
- **腳本聯動**: 特效通常掛載在法術（Spell）或附魔（Enchantment）的 `Visual Effect` 字段中。

---

## 3. 特效運作流程

1.  **觸發**: 玩家施法或物品命中。
2.  **實例化**: 引擎創建一個 `RE::ReferenceEffect`。
3.  **加載**: `ModelLoader` 異步加載對應的特效 NIF。
4.  **掛載**: 引擎將特效模型連接到目標 Actor 的骨骼節點（如：`NPC Head` 或 `NPC 手部`）。
5.  **播放與銷毀**: 特效播放完畢後，引擎自動從 Scene Graph 中移除該節點並回收內存。

---

## 4. C++ 插件開發中的操作方式

### A. 手動施加特效
你可以繞過法術系統，直接給一個 Actor 施加視覺效果。

```cpp
void ApplyEffect(RE::Actor* a_target, RE::BGSVisualEffect* a_vfx) {
    if (a_target && a_vfx) {
        // 使用 ReferenceEffect 管理器
        // 原始碼: include/RE/R/ReferenceEffect.h
        a_target->ApplyVisualEffect(a_vfx, 10.0f); // 持續 10 秒
    }
}
```

### B. 操作正在播放的特效
獲取特效的 3D 模型並修改其顏色。

```cpp
void TintEffect(RE::ReferenceEffect* a_effect) {
    auto vfxNode = a_effect->Get3D();
    if (vfxNode) {
        // 遍歷粒子系統節點，修改頂點顏色或 Shader 屬性
    }
}
```

---

## 5. 核心類別原始碼標註

- **`RE::BGSVisualEffect`**: `include/RE/B/BGSVisualEffect.h` - 靜態數據。
- **`RE::ReferenceEffect`**: `include/RE/R/ReferenceEffect.h` - 運行時實例。
- **`RE::ModelReferenceEffect`**: `include/RE/M/ModelReferenceEffect.h` - 基於模型的特效控制。
- **`RE::ShaderReferenceEffect`**: `include/RE/S/ShaderReferenceEffect.h` - 基於全屏或蒙皮 Shader 的特效。

---

## 6. 技術總結
1.  **想改特效樣子**：修改 `Effects` 文件夾下的 NIF 或貼圖。
2.  **想改觸發條件**：在 ESP 中修改 `Magic Effect` 的關聯項目。
3.  **想用插件控制**：操作 `ApplyVisualEffect` 接口或直接介入 `ReferenceEffect` 列表。
