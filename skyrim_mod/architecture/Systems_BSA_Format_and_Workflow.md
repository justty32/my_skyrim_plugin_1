# Skyrim 檔案封裝深度解析：BSA 格式與虛擬檔案系統運作流程

BSA (Bethesda Archive) 是 Skyrim 引擎使用的二進制資源封裝格式。它不僅僅是一個「壓縮包」，更是一個針對快速隨機訪問優化的**虛擬檔案系統 (Virtual File System, VFS)**。

---

## 1. BSA 的內部構造 (File Structure)

BSA 文件為了實現極速查找，使用了基於「路徑哈希 (Hashing)」的樹狀結構：

### A. Header (文件頭)
- **版本號**: Skyrim SE 通常為 105 (0x69)。
- **標誌位 (Flags)**: 定義是否壓縮、是否包含目錄名/文件名、是否按類型排列。
- **偏移量**: 指向目錄記錄、文件記錄和文件名表的起始位置。

### B. Folder Records (目錄記錄)
- 存儲每個目錄的名稱哈希值（如 `textures\actors`）以及該目錄下文件的起始索引。

### C. File Records (文件記錄)
- 存儲單個文件的名稱哈希值、大小以及在 BSA 中的二進制偏移坐標。

### D. File Data (數據塊)
- 實際的二進制內容。如果開啟了壓縮，數據塊會使用 **Zlib** 或 **LZ4** 進行壓縮。

---

## 2. 運作流程：從硬盤到引擎 (The Workflow)

當遊戲啟動並讀取一個帶有 `.bsa` 的插件（ESP）時，會經歷以下步驟：

### 第一步：掛載 (Mounting)
引擎的 `RE::BSResource::Archive` 管理器會打開 BSA 文件的句柄，讀取 Header 和所有的哈希記錄，並在內存中建立一張「**哈希 -> 偏移量**」的快速查詢表。

### 第二步：路徑解析 (Path Resolution)
當代碼請求 `"Textures\Sky\Cloud.dds"` 時：
1.  引擎將字符串轉換為兩個 64 位哈希值（路徑哈希 + 文件名哈希）。
2.  在掛載的所有 BSA 哈希表中進行對比。

### 第三步：優先級判定 (Override Logic)
這是 Skyrim 模組運作的核心：
1.  **Loose Files (鬆散文件)**: 引擎優先檢查 `Data/` 目錄下是否存在實體文件。
2.  **BSA 覆蓋**: 如果多個 BSA 包含同一個文件，**加載順序靠後 (Load Order)** 的插件對應的 BSA 會覆蓋之前的。

### 第四步：流讀取與解壓 (Streaming)
如果文件在 BSA 中：
1.  引擎跳轉到指定的偏移量。
2.  如果標誌位顯示已壓縮，引擎會調用解壓庫將數據還原到內存緩衝區。

---

## 3. C++ 插件開發中的 BSA 交互

透過 `RE::BSResource` 命名空間，你可以檢測文件是否存在於 BSA 中：

```cpp
auto resourceDB = RE::BSResource::EntryDB::GetSingleton();
const char* filePath = "meshes\\actors\\character\\skeleton.nif";

// 引擎會自動幫你處理：它是來自 BSA 還是 Loose Files
if (RE::BSResource::FileSystem::GetSingleton()->GetEntry(filePath)) {
    // 文件存在，可以安全加載
}
```

---

## 4. 為什麼 BSA 對性能有益？

- **減少磁盤尋道**: 讀取一個巨大的連續文件比讀取一萬個散亂的小文件快得多。
- **內存映射**: 引擎可以利用 Windows 的 `CreateFileMapping` 讓多個資源共用同一個文件緩衝區。

---

## 5. 核心類別原始碼標註

- **`RE::BSResource::Archive`**: `include/RE/B/BSResourceArchive.h` - 單個封裝包的處理類。
- **`RE::BSResource::FileSystem`**: `include/RE/B/BSResourceFileSystem.h` - 虛擬檔案系統總控。
- **`RE::BSResource::Stream`**: `include/RE/B/BSResourceStream.h` - 數據讀取流。
- **`RE::BSResource::Entry`**: `include/RE/B/BSResourceEntry.h` - 檔案項紀錄。
