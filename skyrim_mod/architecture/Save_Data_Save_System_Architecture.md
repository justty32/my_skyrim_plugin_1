# Skyrim 存檔系統架構：數據持久化與 ESS 結構

Skyrim 的存檔系統（Save System）是引擎中最脆弱也最複雜的部分。它負責記錄自遊戲開始以來，世界發生的所有「偏離原始設定」的變化。

---

## 1. 兩層數據結構：Base vs. Change

理解存檔的核心在於區分兩種數據：

### A. 靜態數據 (Base Data - ESP/ESM)
- **原始碼**: `include/RE/T/TESDataHandler.h`
- 當你啟動遊戲時，引擎讀取所有的插件。這些是只讀的藍圖。
- 如果一個物品在 ESP 裡定義為傷害 10，那麼除非存檔裡有覆蓋記錄，否則它永遠是 10。

### B. 動態數據 (Change Data - ESS 存檔)
- **原始碼**: `include/RE/B/BGSLoadGameBuffer.h` & `include/RE/B/BGSSaveGameBuffer.h`
- 存檔文件（.ess）本質上是一系列的「補丁（Patches）」。
- 它只存儲**發生過變化的東西**（Change Forms）。

---

## 2. 世界物件是如何存儲的？

當你保存遊戲時，引擎遍歷世界中的 `RE::TESObjectREFR`，並根據以下規則決定是否寫入存檔：

### A. 持久化對象 (Persistent Objects)
- 如果一個對象被標記為 `Persistent`（或者是獨特 NPC），它的座標、狀態、背包內容會**無條件寫入存檔**。
- 這類對象過多會導致存檔體積膨脹（Save Bloat）。

### B. 臨時對象 (Created/Temporary Objects)
- 透過 `PlaceAtMe` 生成的物體（FormID 為 0xFF 開頭）。
- 這些對象的所有數據都會完整寫入存檔，直到它們被 `SetDelete(true)` 標記並由引擎回收。

### C. ChangeFlags (變化標記)
- 引擎為每個 `TESForm` 維護一組 `ChangeFlags`。
- 如果你只是移動了一個木桶，存檔只會記錄它的「新座標」，而不會記錄它的模型或名稱。

---

## 3. 存檔的內部區塊 (Save Chunks)

一個 `.ess` 文件分為多個區塊：
1.  **Header**: 玩家等級、位置、存檔時間。
2.  **FormID Map**: 插件列表與 FormID 的映射表（確保加載順序改變後存檔依然有效）。
3.  **Change Forms**: 記錄了所有發生變化的 Form 數據。
4.  **Global Variables**: 全球變數的當前值。
5.  **Script Data (VM)**: Papyrus 虛擬機的狀態，包括所有正在運行的腳本變量和堆棧。

---

## 4. SKSE 共用存檔 (Co-Saves / .skse)

由於 `.ess` 的格式是硬編碼的，C++ 插件無法直接往裡面寫入自定義數據。
- **機制**: SKSE 會在同名目錄下生成一個 `.skse` 文件。
- **功能**: 插件可以透過 `SKSE::SerializationInterface` 註冊回調，將自定義的 C++ 結構體序列化到這個文件中。

---

## 5. 技術總結：開發者的責任

- **不要濫用 Persistent**: 除非必要，否則動態生成的物體應盡量保持非持久化，並確保及時回收。
- **序列化安全**: 在 C++ 中存儲指向 Form 的指針時，絕對不能存儲內存地址，必須存儲 `FormID`。因為下次讀取存檔時，內存地址會完全改變。
- **處理加載順序**: 永遠使用 `SKSE::SerializationInterface::ResolveFormID` 來處理因插件順序變化導致的 ID 漂移。

---

## 6. 核心類別原始碼標註

- **`RE::BGSSaveLoadManager`**: `include/RE/B/BGSSaveLoadManager.h` - 存檔流程總控。
- **`RE::BGSSaveFormBuffer`**: `include/RE/B/BGSSaveFormBuffer.h` - 寫入單個 Form 數據的緩衝區。
- **`RE::BGSLoadFormBuffer`**: `include/RE/B/BGSLoadFormBuffer.h` - 讀取數據時的緩衝區。
- **`SKSE::SerializationInterface`**: `include/SKSE/Interfaces.h` - 插件自定義存儲接口。
