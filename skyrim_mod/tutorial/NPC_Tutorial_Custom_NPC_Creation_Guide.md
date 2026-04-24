# 實戰教學：自製特殊角色 (NPC) 開發指南 (Custom NPC)

本教學將教你如何從頭建立一個具備獨立外觀、自定義 AI 與社會關係的特殊角色。

## 難度等級與準備工作
- **難度**: 中階 (Medium)
- **準備工具**:
    - **Creation Kit (CK)**: 核心角色編輯器。
    - **NifSkope**: 處理 FaceGen 模型。

---

## 實作步驟

### 步驟一：建立基礎 NPC
1. 在 CK 的 `Actors` 類別中點擊 New。
2. 設置 `ID`、`Name` 以及 `Race`（種族）。
3. 勾選 `Unique`（唯一角色），這對劇情角色非常重要。

### 步驟二：配置 AI 行為包 (Packages)
1. 在 `AI Packages` 標籤頁添加行為。
2. 建議添加一個 `Sandbox` Package，指定在某個地點周邊互動。
3. 設置時間（例如：20:00 - 08:00 睡覺，08:00 - 20:00 在市場閒晃）。

### 步驟三：製作外觀與導出 FaceGen
1. 在 `Character Gen Parts` 標籤頁調整臉型、髮型與顏色。
2. 在演員清單中選中你的 NPC，按下 `Ctrl + F4`。
3. 這會生成 NIF 與 DDS 檔案，解決「黑臉」問題。

### 步驟四：設置陣營 (Factions)
1. 加入 `PlayerFaction`（如果他是夥伴）。
2. 加入 `TownWhiterunFaction`（如果他是白漫城的居民）。
3. 設置 `Relationship` 以決定他對玩家的好感度。

---

## 代碼實踐 (C++ 動態修改 NPC 屬性)

```cpp
void PowerUpMyNPC(RE::Actor* a_actor) {
    auto base = a_actor->GetActorBase();
    if (base && base->IsUnique()) {
        // 增加生命值上限
        base->AddActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kPermanent, RE::ActorValue::kHealth, 100.0f);
        
        // 修改等級限制
        base->SetLevelMult(2.0f); // 玩家等級的兩倍
        RE::ConsoleLog::GetSingleton()->Print("NPC '%s' 已強化！", a_actor->GetFullName());
    }
}
```

---

## 常見問題與驗證
- **驗證方式**: 在遊戲中按 `~` 打開控制台，輸入 `help "NPC姓名"` 獲取 ID，再輸入 `player.placeatme [ID]` 生成他，觀察其行為。
- **問題 A**: NPC 站在原地不動？
    - *解決*: 檢查 `AI Packages` 是否有正確的 `Location`。如果指定了一個找不到的 Marker，NPC 就會發呆。
- **問題 B**: 黑臉問題？
    - *解決*: 務必執行 `Ctrl + F4` 導出 FaceGen 數據，並確保這些檔案隨模組一起打包。
- **提示**: 給予 NPC 獨特的 `Class` 與 `Combat Style`，能讓他在戰鬥中表現得與眾不同。
