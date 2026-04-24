# Skyrim NPC 反應機制：觀察、過濾與自動對話

當你全裸跑過街頭，或者在城鎮中拔出武器，NPC 會立刻做出評述。這套「反應系統」是透過感知（Detection）、故事管理器（Story Manager）與對話系統（Dialogue）聯動實現的。

---

## 1. 觸發源：觀察與檢測 (Observation)

NPC 不會無緣無故說話，他們必須首先「感知」到事件。
- **偵測等級**: NPC 的 `AIProcess` 每一幀都在掃描周圍的 Actor。
- **原始碼**: `include/RE/D/DetectionData.h`
- **內置觸發器**: 引擎內置了許多「偵測回調」，例如：
    - `OnItemSteal`: 偵測到偷竊。
    - `OnWeaponDraw`: 偵測到拔刀。
    - `OnActorHealthChange`: 偵測到血量變化。

---

## 2. 邏輯分配：故事管理器 (BGSStoryManager)

這是反應系統的「調度中心」。
- **原始碼**: `include/RE/B/BGSStoryManager.h`
- **運作流程**:
    1.  發生事件（如玩家血量變低）。
    2.  引擎向故事管理器發送一個 **Event Node**。
    3.  管理員搜索所有關聯的 **Radiant Dialogue**。
    4.  **條件過濾 (Conditions)**: 這是關鍵！引擎會過濾掉不符合條件的 NPC。
        - 條件 1: NPC 必須活著。
        - 條件 2: NPC 必須能看到玩家。
        - 條件 3: NPC 的性格（Aggression）必須是平和的。

---

## 3. 語音輸出：自動對話 (ForceGreet / Hello)

一旦選定了符合條件的 NPC：
- **`RE::MenuTopicManager`**: 負責將對話內容（`TESTopicInfo`）發送給該 NPC。
- **`Say()` 函數**: 引擎調用 `NPC->Say(TESTopicInfo*)`。
- **視覺聯動**: NPC 自動播放 `TalkingStart` 動畫，並將頭部轉向（LookIK）玩家。

---

## 4. C++ 插件中的精確介入

透過 C++，你可以實現比 ESP 更複雜的反應邏輯：
- **自定義掃描**: 每一幀掃描玩家周圍 1000 距離內的 NPC。
- **精確過濾**: 透過 C++ 檢查 NPC 的性別、年齡或特定變量。
- **強行發話**: 直接調用 API 讓 NPC 開口。

## 5. 核心類別原始碼標註
- **`RE::DetectionData`**: `include/RE/D/DetectionData.h` - 感知數據。
- **`RE::MenuTopicManager`**: `include/RE/M/MenuTopicManager.h` - 語音分發。
- **`RE::Actor::Say`**: `include/RE/A/Actor.h` - 讓 Actor 開口的底層函數。
