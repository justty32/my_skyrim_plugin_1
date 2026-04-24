# 步進式教學：將 MMD 模型轉換並導入 Skyrim (Tutorial)

本教學將帶領你完成從 `.pmx` 檔案到遊戲內可用 NPC 模型的完整流程。

---

## 難度等級：進階 (Advanced)

### 準備工具：
1. **Blender**: 搭配 **mmd_tools** 插件。
2. **Outfit Studio**: 用於骨架權重轉移的核心工具。
3. **NifSkope**: 用於最後的材質與節點校正。
4. **Creation Kit (CK)**: 用於在遊戲中實裝。

---

## 步驟一：Blender 中的預處理
1.  **導入 PMX**: 使用 mmd_tools 導入你的 MMD 模型。
2.  **清理模型**: 刪除所有 MMD 特有的物理鋼體 (RigidBodies) 與關節 (Joints)。
3.  **座標對齊**: 確保模型面向 Y 軸正方向，且雙腳站在座標原點 `(0,0,0)`。
4.  **匯出 FBX**: 選擇「僅幾何體」與「變形骨架」，匯出為 `.fbx`。

---

## 步驟二：Outfit Studio 權重轉移 (關鍵步驟)
1.  **加載參考物件**: 開啟 Outfit Studio，選擇 `File -> New Project`，從現有的 Skyrim 護甲（如 `Body.nif`）作為 `Reference`。
2.  **導入你的 FBX**: 選擇 `File -> Import -> FBX` 導入你的 MMD 模型。
3.  **權重轉移**:
    - 選中你的 MMD 網格。
    - 點擊 `Shape -> Copy Bone Weights`。這會讓 MMD 模型自動獲得 Skyrim 骨骼的權重。
4.  **手動修正**: 檢查腋下、胯下等區域，確保動作時不會穿模。
5.  **匯出 NIF**: 匯出為 `MyMMDModel.nif`。

---

## 步驟三：NifSkope 材質校正
1.  **開啟 NIF**: 用 NifSkope 打開剛匯出的檔案。
2.  **修正貼圖路徑**: 將 `BSShaderTextureSet` 中的路徑指向 `Data/Textures/YourMod/`。
3.  **設置著色器**:
    - 確保 `BSLightingShaderProperty` 的 `Shader Type` 設為 `Default`。
    - 勾選 `Skinned` 屬性（否則模型不會跟著骨架動）。

---

## 步驟四：Creation Kit 遊戲實裝
1.  **建立 ArmorAddon (ARMA)**: 指向你的 `MyMMDModel.nif`。
2.  **建立 Armor (ARMO)**: 將 ARMA 加入其中。
3.  **賦予 NPC**: 
    - 找到你要修改的 NPC。
    - 在 `Inventory` 標籤中加入該 Armor。
    - 在 `Outfit` 標籤中選擇包含該 Armor 的套裝。

---

## 常見問題與驗證 (FAQ)
- **問題：NPC 變成透明的？**
    - **原因**: NIF 檔案中的 `BSLightingShaderProperty` 設置錯誤，或者 `NiTriShape` 遺失。
- **問題：模型變成一坨奇怪的形狀？**
    - **原因**: 權重轉移失敗，頂點沒有正確綁定到骨骼。
- **驗證方法**: 進入遊戲後，打開控制台點擊 NPC，輸入 `resurrect` 強制重新加載其外觀。

---
*文件路徑：architectures/classified/3D_Graphics/Tutorials/Tutorial_MMD_to_Skyrim_Conversion.md*
