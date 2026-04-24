# 06. 終極挑戰：經濟與人口管理 (Economy & Population)

城鎮不能只有空殼，它需要有領主、商人和巡邏的衛兵。本教學將展示如何動態分配人口，並建立一個運作中的「商箱（Merchant Container）」系統，讓你的城市產生經濟活動。

## 1. 核心邏輯
1.  生成不同職業的 NPC 實體（見教學 18）。
2.  生成一個「隱藏的箱子」作為商人的庫存。
3.  將商人所屬的派系（Faction）與商箱綁定。
4.  給 NPC 分配城鎮範圍的沙盒（Sandboxing）行為包。

## 2. 代碼實現：建立鐵匠鋪

```cpp
#include <RE/Skyrim.h>

void SetupBlacksmith(RE::TESObjectREFR* a_forgeRef, RE::BGSLocation* a_townLoc) {
    auto player = RE::PlayerCharacter::GetSingleton();

    // 1. 生成鐵匠 NPC (PlaceHolder: 鐵匠模板 ID)
    auto smithBase = RE::TESForm::LookupByID<RE::TESNPC>(0x00012345);
    auto smith = player->PlaceAtMe(smithBase, 1, true, false)->As<RE::Actor>();

    // 2. 生成商箱 (通常放置在地下，玩家摸不到的地方)
    auto chestBase = RE::TESForm::LookupByID<RE::TESBoundObject>(0x00023456); // 鐵匠商箱
    auto chest = player->PlaceAtMe(chestBase, 1, true, false);
    
    auto forgePos = a_forgeRef->GetPosition();
    forgePos.z -= 500.0f; // 埋入地下 500 單位
    chest->SetPosition(forgePos);

    if (smith && chest) {
        // 3. 關聯派系與商箱 (簡化概念)
        // 實際上，商箱的所有權是透過派系（Faction）鏈接的。
        // 你需要獲取一個商人派系 (JobBlacksmithFaction)，將 NPC 加入該派系，
        // 並將箱子的 Ownership 設為該派系。
        auto smithFaction = RE::TESForm::LookupByID<RE::TESFaction>(0x00034567);
        smith->SetFactionRank(smithFaction, 1);
        
        auto extraOwnership = new RE::ExtraOwnership(smithFaction);
        chest->extraList.Add(extraOwnership);

        // 4. 分配行為：讓鐵匠在熔爐工作
        // 這需要分配一個綁定到 a_forgeRef 或 a_townLoc 的 Sandbox Package
        // ... (參考教學 23 擴充沙盒行為)

        RE::DebugNotification(fmt::format("鐵匠 {} 已入駐城鎮並開始營業！", smith->GetName()).c_str());
    }
}
```

## 3. 人口管控 (Population Control)
-   **派系 (Factions)**: 這是管理人口的核心。創建城鎮時，你需要為其分配一個專屬的「城鎮居民派系」和「衛兵派系」。當城鎮遭到攻擊時，派系系統會決定誰該幫忙戰鬥、誰該逃跑。
---

## 4. 原始碼參考與核心函數

### 核心頭文件
- `include/RE/T/TESNPC.h` - NPC 數據模板。
- `include/RE/T/TESFaction.h` - 派系系統。
- `include/RE/E/ExtraOwnership.h` - 財產所有權。
- `include/RE/A/AIProcess.h` - 行為包執行。

### 推薦使用的函數
- `RE::Actor::SetFactionRank(Faction, Rank)`：將 NPC 納入城鎮管理體系。
- `RE::TESObjectREFR::AddObjectToContainer(...)`：為商箱填充貨物。
- `RE::Actor::EvaluatePackage(bool, bool)`：啟動商人的沙盒工作行為。
- `RE::BGSLocation::HasKeyword(Keyword)`：檢查 NPC 是否處於正確的城鎮區域。