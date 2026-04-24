# Skyrim 架構解析：NIF 加載時機與資源管理系統

3D 模型（NIF）是遊戲中最沉重的數據。為了防止遊戲卡頓，Skyrim 引擎採用了一套複雜的**異步加載**與**引用計數緩存**系統。

---

## 1. NIF 在何時加載？ (The Loading Triggers)

引擎不會在遊戲啟動時加載所有模型，而是根據「需求」動態加載：

### A. 單元切換 (Cell Load)
- 當玩家進入一個室內單元或移動到新的室外網格時，引擎掃描該區域內所有的 `REFR`。
- 引擎會為每個 `REFR` 的 `BaseObject` 查找模型路徑，並將其推入**加載隊列**。

### B. 距離偵測 (LOD to Full Mesh)
- 當玩家靠近一個遠景物件時，引擎會卸載低模（LOD），並即時加載精細的 NIF 模型。

### C. 動態生成 (Spawn)
- 當你調用 `PlaceAtMe` 或施放法術生成投射物時，引擎會立刻觸發加載。如果模型不在緩存中，你可能會看到物體短暫地呈透明狀態（等待加載）。

---

## 2. 資源管理系統：`BSResource` 與緩存

Skyrim 使用 `RE::BSResource` 系統來管理所有的外部文件。

- **虛擬路徑映射**: 引擎會同時檢查 `Data/` 文件夾（Loose Files）和 `BSA` 壓縮包。
- **緩存機制 (Caching)**: 
    - 引擎維護著一張「已加載模型表」。
    - 如果 10 個衛兵都穿著同樣的護甲，引擎只會從磁盤讀取一次 NIF，然後在內存中多次實例化（Clone）。
- **引用計數 (Reference Counting)**:
    - 當一個模型不再被任何世界對象引用時，它不會立刻被刪除，而是保留在緩存中。
    - 只有在內存緊張或進行大規模單元卸載時，`Resource Garbage Collector` 才會清理過期的緩存。

---

## 3. 加載線程：`QueuedFile` 系統

為了保證幀率穩定，NIF 的加載通常發生在**非遊戲主線程**上：
- **IO Thread**: 負責從磁盤/BSA 讀取原始二進制數據。
- **Task Thread**: 負責解析 NIF 的 Block 結構並建立初始節點樹。
- **Main Thread**: 最後將解析好的模型掛載到遊戲世界中（這就是 `Update3DModel` 的執行點）。

---

## 4. C++ 插件開發啟發：如何正確請求模型？

如果你想在插件中預加載一個模型，防止生成時卡頓：

```cpp
void PreloadModel(const char* a_path) {
    // 透過文件管理器獲取流
    auto loader = RE::BSResource::FileSystem::GetSingleton();
    
    // 這種方式可以讓模型提前進入緩存
    // ... (底層涉及 QueuedFile 請求)
}
```

**警告**: 不要在 `Main::Update` 循環中直接調用阻塞式的讀取函數，這會讓玩家的遊戲每秒卡頓幾次。

---

## 5. 核心類別原始碼標註

- **`RE::BSResource::EntryDB`**: `include/RE/B/BSResourceEntryDB.h` - 資源緩存清單。
- **`RE::ModelLoader`**: `include/RE/M/ModelLoader.h` - 高層級的模型加載接口。
- **`RE::QueuedFile`**: `include/RE/Q/QueuedFile.h` - 異步文件請求封裝。
- **`RE::BSResource::Archive`**: `include/RE/B/BSResourceArchive.h` - BSA 文件處理。
