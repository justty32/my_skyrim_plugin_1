# Skyrim 架構解析：NPC 的定義、實例化與存檔序列化

NPC（角色）是 Skyrim 中數據結構最龐大、存儲最複雜的對象。它們的生命始於 ESP 中的靜態藍圖，演變為內存中的動態實體，最後以「差異快照」的形式存在於存檔中。

---

## 1. 靜態定義：NPC 藍圖 (NPC_ - `TESNPC`)

在數據文件（ESP/ESM）中，每個角色都由一個 `NPC_` 記錄定義。
- **原始碼**: `include/RE/T/TESNPC.h`
- **儲存內容**:
    - **生理基礎**: 種族（Race）、性別、身高、體重。
    - **屬性與技能**: 基礎生命/法力值、技能等級、成長職業（Class）。
    - **外觀數據**: FaceGen 頂點數據、膚色、髮型、化妝貼圖。
    - **初始配置**: 初始背包物品、所屬派系（Factions）、初始法術、AI 行為包（Packages）。

---

## 2. 實例化：活動實體 (Actor - `RE::Actor`)

當遊戲需要一個 NPC 出現在世界中時，引擎會創建一個 `RE::Actor` 實體。
- **原始碼**: `include/RE/A/Actor.h`
- **結構**: 一個 `Actor` = `TESNPC*` (指向藍圖) + `Runtime State` (運行時狀態)。
- **運行時數據**:
    - **座標與物理**: 當前位置、旋轉、當前正在播放的動畫。
    - **動態屬性**: 當前血量（受傷後的值）、當前魔力、當前耐力。
    - **裝備狀態**: 當前穿在哪件護甲、左/右手分別拿著什麼。
    - **AI 進程**: `AIProcess` 記錄了 NPC 當前正在看誰、仇恨值是多少。

---

## 3. 存檔中的模樣：`ChangeForm` 與持久化

當你保存遊戲（ESS）時，NPC 的數據會根據其「重要程度」進行序列化：

### A. 獨特 NPC (Unique NPCs)
- **儲存策略**: 他們的 `Actor` 實體幾乎所有的變化都會被記錄。
- **存檔中的內容**:
    - **座標**: 他們在哪個單元、哪個坐標。
    - **死亡標記**: 他們是否已死亡。
    - **背包變化**: 他們拿走了玩家給的什麼東西，或者丟棄了什麼。
    - **好感度**: 對玩家的關係值（Relationship Rank）。

### B. 隨機路人 (Leveled NPCs)
- **儲存策略**: 只有當他們與原始狀態不同（如：受傷、位移）且所在的單元尚未重置時，才會存儲。
- **序列化標誌 (`ChangeFlags`)**:
    - `kChanged_Actor_Stats`: 屬性發生過變化。
    - `kChanged_Inventory`: 背包發生過變化。
    - `kChanged_Position`: 位置發生過變化。

---

## 4. NPC 序列化的二進制細節

在 ESS 文件中，一個實例化後的 NPC 序列化後大約長這樣（二進制結構）：
1.  **FormID**: 該 NPC 的唯一標識。
2.  **Change Flags**: 一個 32 位掩碼，告訴引擎接下來要讀取哪些數據。
3.  **Data Payload**:
    - 如果位置變了：[X, Y, Z, Rot]。
    - 如果屬性變了：[HP_Current, MP_Current, SP_Current]。
    - 如果背包變了：[Added Items List] + [Removed Items List]。

---

## 5. C++ 插件開發啟示

- **持久性操作**: 如果你用 C++ 生成一個 NPC 並希望他在存檔中保留，必須獲取他的 `Handle` 並確保他沒有被 `SetDelete(true)`。
- **外觀修改**: 在運行時修改 `TESNPC` 的外觀（如換頭髮）不會自動更新已經實例化的 `Actor`，通常需要調用 `Update3DModel()` 並重新加載。
- **性能**: 存檔讀取 NPC 時，引擎會執行大量的 `LookupByID`。在 C++ 插件中，若需要大量追蹤 NPC，建議使用 `RE::ObjectRefHandle` 以保證安全。

---

## 6. 核心類別原始碼標註

- **`RE::TESNPC`**: `include/RE/T/TESNPC.h` - 靜態藍圖。
- **`RE::Actor`**: `include/RE/A/Actor.h` - 活動實體。
- **`RE::AIProcess`**: `include/RE/A/AIProcess.h` - NPC 的運行時決策內存。
- **`RE::BGSSaveFormBuffer`**: `include/RE/B/BGSSaveFormBuffer.h` - NPC 寫入存檔的接口。
