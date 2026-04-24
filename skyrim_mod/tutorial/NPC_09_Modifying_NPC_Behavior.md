# 09. 進階實戰：修改 NPC 行為與 AI 邏輯

本教學將教您如何通過 C++ 插件動態干預 NPC 的大腦。我們將實現一個功能：**當玩家對 NPC 使用特定龍吼時，強迫該 NPC 停止戰鬥、跪下投降，並將其侵略性降為最低。**

## 1. 核心邏輯
1.  攔截龍吼事件。
2.  獲取受影響的 `Actor`（NPC）。
3.  修改其 **Actor Values**（侵略性、自信度）。
4.  清除其當前的戰鬥狀態。
5.  播放「投降」動畫。

## 2. 代碼實現

```cpp
void TameNPC(RE::Actor* a_target) {
    if (!a_target || a_target->IsPlayerRef()) return;

    // 1. 修改 Actor Values 改變性格
    // 原始碼路徑: include/RE/A/ActorValueOwner.h
    
    // 侵略性 (Aggression): 0-平和, 1-侵略, 2-激進, 3-瘋狂
    a_target->SetActorValue(RE::ActorValue::kAggression, 0.0f);
    
    // 自信度 (Confidence): 0-懦弱 (會逃跑), 4-魯莽 (永不逃跑)
    a_target->SetActorValue(RE::ActorValue::kConfidence, 0.0f);

    // 2. 強制停止戰鬥
    // 原始碼路徑: include/RE/A/Actor.h
    a_target->StopCombat();
    
    auto combatGroup = a_target->GetCombatGroup();
    if (combatGroup) {
        // 從戰鬥組中移除目標
    }

    // 3. 播放動畫 (跪下)
    // 我們可以通過發送一個動畫事件 (Animation Event)
    // 這裡使用內建的 "Yield" 或 "Bleedout" 動畫標籤
    a_target->NotifyAnimationGraph("BleedoutStart");

    // 4. 修改派系關係 (使其不再敵對)
    // PlaceHolder: 0x玩家派系ID
    auto playerFaction = RE::TESForm::LookupByID<RE::TESFaction>(0x0000001C);
    if (playerFaction) {
        a_target->SetFactionRank(playerFaction, 1);
    }

    // 5. 強制重新評估行為
    // 這會讓 NPC 檢查是否有新的合法 Package 需要執行
    a_target->EvaluatePackage(false, true);

    RE::DebugNotification(fmt::format("{} 已投降。", a_target->GetName()).c_str());
}
```

## 3. 進階：給予 NPC 新的行為包 (Package)

如果你想讓 NPC 跟隨你，或者走向特定的地方，你需要動態添加一個 `TESPackage`。

```cpp
void ForceFollowPlayer(RE::Actor* a_target) {
    // 獲取一個預設的跟隨 Package (PlaceHolder)
    auto followPackage = RE::TESForm::LookupByID<RE::TESPackage>(0x000595B1);
    
    if (a_target && followPackage) {
        // 將 Package 推入 NPC 的行為棧
        auto process = a_target->currentProcess;
        if (process) {
            process->SetCurrentPackage(followPackage);
        }
        
        a_target->EvaluatePackage(true, false);
    }
}
```

## 4. 關鍵 API 標註
-   **`SetActorValue()`**: 修改 NPC 的屬性（侵略性、自信度、士氣）。`include/RE/A/ActorValueOwner.h`
-   **`StopCombat()`**: 立刻停止當前的戰鬥行為。`include/RE/A/Actor.h`
-   **`NotifyAnimationGraph()`**: 向動畫機發送信號，執行特定動作。`include/RE/I/IAnimationGraphManagerHolder.h`
-   **`EvaluatePackage()`**: 讓 NPC “重新思考”現在該做什麼。`include/RE/A/Actor.h`

## 5. 實戰建議
-   **持久性**: 僅通過 `SetActorValue` 修改的屬性會保存在存檔中。如果你希望效果是暫時的，請記住在一段時間後將屬性還原。
-   **動畫狀態**: 播放 `BleedoutStart` 會讓 NPC 進入倒地受傷狀態，直到你發送 `BleedoutStop` 或其生命值恢復。
-   **戰鬥組**: 如果 NPC 所在的整個小隊都在攻擊你，僅停止一個 NPC 的戰鬥可能不夠，他可能會被同伴重新拉入戰鬥。
