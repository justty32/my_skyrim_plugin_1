# Skyrim NPC 生命週期與類別解析：獨特 NPC vs 隨機路人

在 Skyrim 引擎中，並非所有 NPC 都是平等的。引擎根據 NPC 的「身份標籤」採用完全不同的存儲、生成與清理策略。

---

## 1. NPC 的兩大分類

### A. 獨特 NPC (Unique NPCs)
- **定義**: 有名字、有故事、通常與任務掛鉤（如：巴爾古夫領主、萊迪亞）。
- **標誌**: `TESNPC` 表單中勾選了 `Unique` 標籤。
- **原始碼**: `include/RE/T/TESNPC.h` (Record Flags)
- **特徵**:
    - **唯一性**: 在整個遊戲世界中只會有一個實例。
    - **持久化**: 他們的動態數據（如：好感度、當前血量、位置）會被永久保存在存檔中。
    - **死亡**: 一旦死亡，除非透過腳本復活，否則不會「刷新（Respawn）」。

### B. 隨機/路人 NPC (Generic / Leveled NPCs)
- **定義**: 強盜、衛兵、野外野獸。
- **機制**: 透過 `TESLevCharacter` (分級清單) 生成。
- **原始碼**: `include/RE/T/TESLevCharacter.h`
- **特徵**:
    - **動態生成**: 玩家進入區域時，根據玩家等級從清單中「抽獎」生成。
    - **非持久**: 當玩家離開區域（Cell Unload）且經過一段時間後，這些 NPC 的數據會被徹底刪除。
    - **刷新**: 區域重置後，會生成全新的實體。

---

## 2. NPC 的生命週期 (The Lifecycle)

### 第一階段：誕生 (Spawning)
1.  **靜態放置**: 在 ESP 中預先擺放的 NPC。
2.  **動態生成**: 透過 `Leveled List` 觸發。
3.  **插件生成**: 透過 C++ 的 `PlaceAtMe` 生成（見教學 18）。

### 第二階段：活動 (Activation)
- **加載 3D**: 當玩家靠近時，引擎讀取 NIF 模型，將數據對象轉化為可見實體。
- **進入 AI 循環**: `AIProcess` 開始每一幀的計算。

### 第三階段：死亡與清理 (Death & Cleanup)
這是區分路人的關鍵點：
1.  **死亡狀態**: 設置 `ActorState::IsDead`。
2.  **屍體清理 (Corpse Cleanup)**:
    - **路人**: 玩家離開 Cell 且遊戲時間經過 `iHoursToRespawnCell`（預設 240 小時）後，引擎直接銷毀該對象。
    - **獨特 NPC**: 屍體通常會保留很久，或者被轉移到專門的「死者之廳（Dead Body Cleanup Cell）」。

---

## 3. 行為計算方式 (AI Calculation)

引擎如何決定一個 NPC 此刻該做什麼？這是一個**權重優先級系統**：

### 1. 任務別名優先 (Alias Packages)
如果 NPC 被某個運行中的任務（Quest）標記為「別名（Alias）」，那麼任務賦予他的行為包（Package）擁有最高優先級。這就是為什麼領主在開會時不會去睡覺。

### 2. 行為包堆疊 (Package Stack)
每個 NPC 都有一個 Package 清單。引擎會從上往下掃描，檢查每個 Package 的 **Conditions（條件）**。
- **條件檢查**: `GetTimeOfDay`, `IsInLocation`, `GetGlobalValue`。
- **第一個符合條件的 Package** 會被推入 `AIProcess` 執行。

### 3. 戰鬥接管 (Combat Overrides)
一旦 `IsInCombat` 為真，常規的行為計算會暫停，控制權移交給 `CombatController`。它會計算尋路、法術選擇和攻擊間隔。

---

## 4. C++ 插件開發啟示

- **有名有姓的 NPC**: 修改他們的數據要非常小心，因為會永久記錄在存檔。
- **路人 NPC**: 適合用來做實驗。你可以批量修改強盜的 AI 而不用擔心弄壞玩家的長期存檔。
- **判斷方式**:
    ```cpp
    bool IsNamedNPC(RE::Actor* a_actor) {
        auto base = a_actor->GetActorBase();
        return base && base->IsUnique();
    }
    ```

## 5. 核心類別原始碼標註

- **`RE::TESNPC`**: `include/RE/T/TESNPC.h` - 數據定義。
- **`RE::Actor`**: `include/RE/A/Actor.h` - 運行時實體。
- **`RE::TESPackage`**: `include/RE/T/TESPackage.h` - 行為指令。
- **`RE::TESLevCharacter`**: `include/RE/T/TESLevCharacter.h` - 隨機路人生成器。
