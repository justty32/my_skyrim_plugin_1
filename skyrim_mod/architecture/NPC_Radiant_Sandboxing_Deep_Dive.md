# 深度解析：Radiant AI 自主沙盒行為 (Sandboxing)

沙盒行為（Sandboxing）是 NPC 在沒有特定任務指令時，表現出“日常生活”狀態的核心機制。

## 1. 核心驅動：Sandbox Package
- **原始碼**: `include/RE/T/TESPackage.h`
- **組成內容**:
    - **Radius / Location**: 定義 NPC 活動的空間邊界。
    - **Valid Markers**: 定義 NPC 可以使用的家具或標記類型。

## 2. 掃描與偵測邏輯
當 NPC 進入 Sandbox 狀態時，`AIProcess` 會執行以下掃描：
1.  **收集對象**: 在定義的半徑內，查找所有的 `RE::TESObjectREFR`。
2.  **標記點過濾 (Markers)**:
    - **Furniture (家具)**: 椅子、床、磨刀石、烹飪鍋。
    - **Idle Markers (閒置點)**: 牆邊靠著的地點、可以扶著的欄杆、可以掃地的區域。
3.  **條件檢查**: 每個標記點都可能有 `Conditions`（例如：只有晚上能用的床，或者只有特定性別能用的點）。

## 3. 動作選擇算法 (The Score System)
NPC 不會隨機選擇動作，而是有一套隱藏的權重計算：
- **距離加權**: 越近的點權重越高。
- **類型傾向**: 飢餓值觸發時，NPC 會優先尋找「食物」或「椅子」。
- **獨佔檢查**: 如果一個標記點已被其他 NPC 佔用，引擎會自動跳過該點。

## 4. C++ 介入與擴展
透過 C++，你可以實現「程序化地標 (Procedural Idle Markers)」：
- **動態生成標記**: 在你的動態城鎮（教學 21）中，手動 `PlaceAtMe` 一些不可見的閒置點（Idle Markers），NPC 就會自動在那裡表現出靠牆、看書或修補建築的動作。
- **修改權重**: Hook `RE::AIProcess::Update` 可以在 NPC 選擇動作前攔截目標對象。

---

## 5. 核心類別原始碼標註
- **`RE::TESPackage`**: `include/RE/T/TESPackage.h` - 定義 Sandbox 範圍。
- **`RE::BGSIdleMarker`**: `include/RE/B/BGSIdleMarker.h` - 定義 NPC 的閒置動作（如：掃地、看地圖）。
- **`RE::TESFurniture`**: `include/RE/T/TESFurniture.h` - 定義物理交互對象（如：椅子、床）。
