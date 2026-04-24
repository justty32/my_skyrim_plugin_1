# 19. 高級實戰：修改 NPC 模板 (基因改造)

修改 `Actor` 實體只會影響一個個體，但修改 `TESNPC` 模板會影響遊戲中**所有**基於該模板生成的 NPC。例如，如果你修改了“強盜”的模板，全天際的強盜都會受到影響。

## 1. 核心邏輯
1.  找到目標 `TESNPC` 模板。
2.  修改其基礎屬性（生命值、法力、耐力偏移）。
3.  修改其技能等級。
4.  （進階）修改其外觀數值（身高、體重）。

## 2. 代碼實現：強化全天際的衛兵

```cpp
void BuffAllGuards() {
    // 1. 獲取衛兵模板 (雪漫城衛兵)
    auto guardBase = RE::TESForm::LookupByID<RE::TESNPC>(0x00012E46);
    
    if (guardBase) {
        // 2. 修改基礎屬性
        // 增加 100 點生命值偏移量
        guardBase->baseID.baseStats.healthOffset = 100;
        
        // 3. 修改身高 (1.0 是標準)
        guardBase->height = 1.1f; 
        
        // 4. 修改技能 (例如：將其防禦技能設為 100)
        // 原始碼: include/RE/T/TESNPC.h -> ActorValue
        // 注意：這需要修改模板的技能字典，通常使用 SetActorValue 
        // 但對於模板，直接修改數據成員更直接
        
        // 5. 強制刷新
        // 已經存在於世界上的 NPC 可能需要重新加載 3D 才能看到身高變化
        RE::DebugNotification("全體衛兵已獲得強化與身高增長。");
    }
}
```

## 3. 關於 FaceData (臉部數據)
修改 NPC 模板的臉部是非常複雜的，涉及 `RE::TESNPC::FaceData` 結構。在純 C++ 中，不建議動態修改臉部頂點，這極易導致遊戲崩潰。

## 4. 關鍵 API 標註
-   **`RE::TESNPC`**: 成員包括 `height`, `weight`, `race`, `baseStats` 等。`include/RE/T/TESNPC.h`
-   **`RE::ActorValue`**: 用於指定要修改的屬性類型。`include/RE/A/ActorValues.h`

## 5. 警告與限制
-   **存檔副作用**: 修改模板數據是「破壞性」的。如果你的插件在運行時修改了模板，這些修改可能會永久滲透進玩家的存檔。
-   **性能**: 修改模板本身不耗性能，但如果你嘗試在修改後立刻遍歷世界上所有實體並執行 `Update3D()`，會造成短暫卡頓。
-   **推薦時機**: 建議在 `kDataLoaded` 事件中進行一次性的模板修改，而不是在每一幀中頻繁變更。
