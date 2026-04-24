# Skyrim 存檔深度解析：ESS 文件結構與 ChangeForms 機制

`.ess` (Elder Scrolls Save) 文件是遊戲世界的「差異備忘錄」。它不存儲整個世界，只存儲自遊戲開始以來，與原始數據（ESP/ESM）相比**發生過變化的部分**。

---

## 1. ESS 文件組成區塊

一個 ESS 文件是由多個二進制區塊（Chunks）組成的，主要包含以下部分：

### A. 文件頭 (Header)
- **內容**: 存檔版本、玩家等級、當前位置、存檔名稱、遊戲時間。
- **作用**: 供加載介面快速顯示基本信息。

### B. 插件列表 (Plugin List)
- **內容**: 存檔那一刻，玩家安裝的所有 ESP/ESM/ESL 文件名及其順序。
- **關鍵機制**: 當你改變加載順序後讀檔，引擎會根據這個清單重新映射所有 `FormID`（這就是為什麼順序變了存檔通常還能用，但刪了插件就會報錯）。

### C. ChangeForms (變化的表單) - 核心部分
這是存檔體積最大的部分。
- **機制**: 引擎會為每個發生變化的 Form（如你移動過的椅子、死掉的強盜）創建一個 `ChangeForm` 記錄。
- **ChangeFlags**: 每個記錄都有一個 32 位的位掩碼，標識哪些字段發生了變化（如：座標變了、背包變了、或是被 Disable 了）。
- **原始碼**: `include/RE/B/BGSChangeForm.h`

### D. 全球變數與表格 (Globals & Map)
- 存儲遊戲中所有 `TESGlobal` 的當前數值。
- 存儲地圖標記的開啟狀態。

### E. Papyrus 腳本數據 (VM Data)
這是存檔最脆弱的部分。
- **內容**: 所有正在運行的腳本實例、變量數值、等待中的 `Wait` 函數、堆棧幀（Stack Frames）。
- **孤兒腳本 (Orphaned Scripts)**: 如果你刪除了一個帶有腳本的模組，存檔裡依然會保留這些腳本的數據和變量。引擎會不斷嘗試運行它們，最終導致「存檔膨脹」或崩潰。

---

## 2. ChangeForms 的運作流程

1.  **偵測變化**: 當玩家在遊戲中與物體交互（如：撿起一個蘋果）。
2.  **標記 Flag**: 該蘋果的 `RE::TESObjectREFR` 被標記為 `kChanged_Inventory`。
3.  **序列化**: 保存時，引擎發現 Flag 被激活，將蘋果的當前狀態寫入 ESS。
4.  **加載回溯**: 讀檔時，引擎先加載 ESP 裡的初始蘋果，然後讀取 ESS 裡的 `ChangeForm` 覆蓋掉初始數據。

---

## 3. C++ 插件與存檔安全

作為 SKSE 插件開發者，你必須遵循以下準則：

### A. 永遠不要在 C++ 類中直接存儲 Raw Pointer
- **原因**: 存檔後重啟，對象的內存地址會完全改變。
- **解決方案**: 存儲 `FormID`，並在讀檔後透過 `LookupByID` 找回。

### B. 使用 SKSE 序列化 (Serialization)
- **原始碼**: `include/SKSE/Interfaces.h` (SerializationInterface)
- 如果你的插件有自定義數據（如：某個 NPC 的特殊心情值），不要嘗試改寫 ESS，應註冊 `SKSE::Serialization` 回調，將數據存入同名的 `.skse` 文件中。

### C. FormID 重定位
- 如果你的插件依賴另一個 ESP，務必使用 `ResolveFormID` 處理插件順序變化導致的 ID 漂移。

---

## 4. 核心類別原始碼標註

- **`RE::BGSSaveLoadManager`**: `include/RE/B/BGSSaveLoadManager.h` - 存檔管理中心。
- **`RE::BGSLoadGameBuffer`**: `include/RE/B/BGSLoadGameBuffer.h` - 讀取緩衝區。
- **`RE::BGSSaveFormBuffer`**: `include/RE/B/BGSSaveFormBuffer.h` - 寫入緩衝區。
- **`RE::ChangeForm`**: `include/RE/C/ChangeForm.h` - 單個變化記錄。
