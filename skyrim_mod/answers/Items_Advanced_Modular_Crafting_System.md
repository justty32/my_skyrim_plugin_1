# Skyrim 高階模組化鍛造系統架構 (Advanced Modular Crafting)

要實作一個能自由選擇「劍身 (Blade)」與「劍柄 (Hilt)」並動態改變模型與數值的系統，需要突破 Skyrim 原生的「單一靜態模型」限制。這通常涉及動態 NIF 合併與自定義數據儲存。

---

## 1. 核心設計：組件化數據

不要將每一種組合都做成一個獨立的武器 Form（這會造成數量爆炸）。應改為：
- **基礎武器 (Base Weapon)**: 一個通用的「模組化長劍」Form。
- **組件 (Components)**: 建立自定義的數據結構或 FormList，定義不同的劍身與劍柄。
    - **劍身 A**: 鋼製，傷害 +10，重量 +5，模型路徑 `Models\Blade_Steel.nif`。
    - **劍柄 B**: 皮革，攻速 +0.1，模型路徑 `Models\Hilt_Leather.nif`。

---

## 2. 技術實作方案：動態模型合併

這是實現「模型隨選擇改變」的關鍵。

### A. 動態 NIF 掛載 (Runtime NIF Attachment)
當玩家裝備該武器時，透過 SKSE 攔截 3D 加載事件：
1. **獲取武器節點**: 找到武器的 `NiNode`。
2. **清除舊模型**: 移除所有子節點。
3. **加載並附加新模型**:
   ```cpp
   // 虛擬 C++ 代碼：動態附加組件
   void AssembleWeapon(RE::NiNode* a_weaponNode, const std::string& a_bladePath, const std::string& a_hiltPath) {
       auto bladeNode = RE::BSFixedString(a_bladePath);
       auto hiltNode = RE::BSFixedString(a_hiltPath);
       
       // 使用 NiStream 加載 NIF 片段
       // 將片斷作為 Child 附加到主武器節點的特定 Marker 上
       a_weaponNode->AttachChild(LoadNif(bladeNode), true);
       a_weaponNode->AttachChild(LoadNif(hiltNode), true);
       
       // 更新 3D 渲染
       a_weaponNode->Update(RE::NiUpdateData{});
   }
   ```

### B. 特殊節點 (Marker Nodes)
在主武器 NIF 中設置 `BladeMarker` 與 `HiltMarker` 節點。這確保了不同形狀的劍身能精確對齊到劍柄上。

---

## 3. 動態數值計算 (Dynamic Stats)

由於基礎武器的傷害是固定的，我們必須動態修改它。

1. **攔截傷害計算**: Hook `RE::TESObjectWEAP::GetDamage()`。
2. **讀取組件數據**: 從該武器實例的 `ExtraDataList` 中讀取保存的組件 ID。
3. **加權計算**: 
   - `最終傷害 = 劍身基礎傷害 * 材質倍率 + 附魔加成`。
   - `最終速度 = 劍柄平衡係數 * 劍身重量比`。

---

## 4. 自定義鍛造介面 (Assembly UI)

原版的鍛造選單無法處理這種複雜選擇。
- **UI 實作**: 製作一個新的 Flash 選單（參考 `Custom_UI_and_Flash_Integration.md`）。
- **預覽功能**: 在 UI 開啟時，在玩家面前生成一個臨時的武器物件，隨玩家選擇即時調用 `AssembleWeapon` 函數，實現即時預覽效果。

---

## 5. 數據持久化 (Persistence)

玩家打造的這把劍必須在讀檔後保持原樣。
- **ExtraData**: 使用 SKSE 的 `ExtraData` 系統為該武器實例附加自定義數據。
- **序列化**: 在存檔時，將 `{BladeID, HiltID, MaterialID}` 寫入存檔。

---

## 6. 核心類別原始碼標註

- **`RE::NiNode`**: `include/RE/N/NiNode.h` - 3D 節點操作。
- **`RE::TESObjectWEAP`**: 武器基礎屬性。
- **`RE::ExtraDataList`**: 儲存實例特有數據。
- **`RE::IFormFactory`**: 動態建立物件。

---
*文件路徑：architectures/classified/Items/Advanced_Modular_Crafting_System.md*
