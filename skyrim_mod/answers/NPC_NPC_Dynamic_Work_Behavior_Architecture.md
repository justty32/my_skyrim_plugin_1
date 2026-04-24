# NPC 動態工作行為擴充架構 (Dynamic Work Behavior)

要實現「NPC 見到曬魚架就去工作」的行為，核心在於利用 Skyrim 的 **AI Package 系統** 搭配 **物件關鍵字 (Keywords)**。

---

## 1. 核心機制：`TESPackage` 與 `Sandbox`

NPC 的所有行為都由 `RE::TESPackage` 驅動。

### A. Sandbox (沙盒程序)
- **邏輯**: NPC 在特定範圍內搜尋「可互動對象」。
- **搜尋優先級**: 
    1.  **Furniture (家具)**: 這是我們最常用的，因為它自帶動畫。
    2.  **Idle Markers (閒置標記)**: 單純播放動作。
    3.  **Items (物品)**: 撿拾或吃掉。

### B. 物件辨識 (Object Detection)
我們不應該讓 NPC 找「特定的曬魚架 A」，而是讓他們找「帶有 `WKS_FishRack` 關鍵字的任何物件」。
- **技術**: 在 Package 的 `Procedure` 中設置 `Type: Furniture` 並過濾 `Keyword: YourCustomKeyword`。

---

## 2. 實作路徑：全局行為注入

如果你希望**所有**漁夫或特定地區的 NPC 都能自動獲得這個行為，有兩種方式：

### 方案 A：陣列包 (Faction Packages)
- **做法**: 建立一個 `FishWorkerFaction`，將曬魚 Package 放入該陣列的 `PackList`。
- **優點**: 只要 NPC 加入這個陣列，就會自動獲得該行為。

### 方案 B：C++ 動態注入 (Package Injection)
- **做法**: 透過 SKSE 攔截 NPC 加載。
- **邏輯**: 
    ```cpp
    void InjectWorkPackage(RE::Actor* a_actor) {
        auto process = a_actor->GetActorRuntimeData().currentProcess;
        if (process) {
            // 檢查 NPC 周圍是否有曬魚架
            if (FindNearbyObject(a_actor, "WKS_FishRack")) {
                // 將曬魚 Package 推入 NPC 的行為堆疊
                a_actor->GetActorRuntimeData().currentProcess->PushPackage(g_FishRackPackage);
            }
        }
    }
    ```

---

## 3. 視覺與動畫：`TESFurniture`

NPC 去曬魚架「工作」實際上是進入了一個特殊的家具狀態。
- **Furniture Marker**: NIF 檔案中必須包含互動點。
- **Animation Tags**: 指定如 `Work` 或 `Idle`。你可以自定義動畫，讓 NPC 播放掛魚、檢查魚乾等動作。

---

## 4. 核心類別原始碼標註

- **`RE::TESPackage`**: `include/RE/T/TESPackage.h` - 行為定義。
- **`RE::TESFurniture`**: `include/RE/T/TESFurniture.h` - 互動設備。
- **`RE::AIProcess`**: 處理 NPC 當前正在執行的行為。
- **`RE::BGSKeyword`**: 用於分類物件。

---
*文件路徑：architectures/classified/NPC/Answers/NPC_Dynamic_Work_Behavior_Architecture.md*
