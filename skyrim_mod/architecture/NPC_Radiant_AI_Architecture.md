# Skyrim Radiant AI 架構：動態任務與自主行為系統

Radiant AI 是 Bethesda 引擎的核心技術之一，旨在讓遊戲世界感覺“活著”。它分為兩個主要子系統：**Radiant Story (動態劇情)** 和 **Radiant Behavior (自主行為)**。

---

## 1. Radiant Behavior：NPC 的日常生活

與傳統遊戲中寫死的腳本不同，Radiant AI 允許 NPC 根據環境動態決定行為。

### A. 需求驅動 (The Need System)
雖然 Skyrim 簡化了 Fallout 中的“渴、餓、睡”系統，但 NPC 依然具備行為傾向。
- **機理**: 引擎會檢查 NPC 所在區域的「家具（Markers）」。如果附近有一把椅子且 NPC 具備 `Sandboxing` 屬性，他會“自主”決定去坐下。

### B. Sandboxing (沙盒行為)
- **原始碼**: `include/RE/T/TESPackage.h` (涉及 `PackageType::kSandbox`)
- **運作邏輯**:
    1.  定義一個半徑（Radius）或區域（Location）。
    2.  NPC 在區域內掃描可交互對象（如：烹飪鍋、磨刀石、椅子）。
    3.  根據 `Procedure Tree` 動態生成一條路徑並執行動作。

---

## 2. Radiant Story：動態任務生成

這是讓遊戲世界響應玩家行為的系統。例如：當你殺死一個 NPC 的配偶，他可能會僱傭殺手來報復。

### A. 故事管理器 (BGSStoryManager)
- **原始碼**: `include/RE/B/BGSStoryManager.h`
- **機制**: 監聽遊戲內的「節點（Nodes）」。常見節點包括：`Kill Actor`, `Level Up`, `Pickpocket`。
- **匹配邏輯**: 當觸發節點時，管理員會掃描所有具備 `Radiant` 標籤的任務，檢查其 **Conditions**。如果條件匹配，任務會自動開始。

### B. 別名填充 (Alias Filling)
- **關鍵**: `RE::BGSRefAlias`
- **機制**: Radiant 任務不需要寫死對象。它可以定義一個別名為“受害者的親屬”。引擎會自動在數據庫中查找符合條件的 NPC 並將其填入任務。

---

## 3. C++ 插件開發中的介入點

透過 C++，你可以極大地擴展 Radiant AI 的能力。

### A. 動態注入觸發節點
你可以手動觸發 `BGSStoryManager` 的節點，強行啟動原本不會發生的動態任務。

```cpp
void TriggerRevengeQuest(RE::Actor* a_victim) {
    auto storyManager = RE::BGSStoryManager::GetSingleton();
    // 注入一個自定義的“犯罪”節點
    // ...
}
```

### B. 擴展沙盒行為
你可以 Hook `RE::TESPackage` 的解析邏輯，讓 NPC 能夠識別並使用你透過插件動態生成的家具（見教學 04/07）。

---

## 4. 為什麼 Radiant AI 有時顯得很笨？

- **導航網格 (Navmesh) 限制**: NPC 的自主權被限制在 Navmesh 覆蓋的範圍內。
- **條件衝突**: 如果一個任務（Quest）強行給 NPC 綁定了高優先級的行為包，Radiant AI 的日常生活行為會被徹底覆蓋。

---

## 5. 核心類別原始碼標註

- **`RE::BGSStoryManager`**: `include/RE/B/BGSStoryManager.h` - 劇情分發器。
- **`RE::TESPackage`**: `include/RE/T/TESPackage.h` - 行為單元。
- **`RE::BGSScene`**: `include/RE/B/BGSScene.h` - 處理 NPC 之間的動態對話與小劇場。
- **`RE::BGSRefAlias`**: `include/RE/B/BGSRefAlias.h` - 任務對象自動綁定。
