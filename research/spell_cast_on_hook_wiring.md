# `spell_cast_on` 偵測：最小接線草圖與核對來源

[返回總索引](spell_cast_on_hook.md)

### 最小接線草圖（接進 `src/skyrim/SkyrimEvents.{h,cpp}` 餵 `spell_cast_on`）

> 以下為實作建議，**本研究任務為唯讀，不修改任何既有檔案**。簽章皆已對照 header。

`SkyrimEvents.h`（新增一個 sink 成員，鏡像現有 `ActivateSink`）：

```cpp
// private:
class MagicApplySink;            // 定義在 .cpp，鏡像 ActivateSink
MagicApplySink* magicApplySink_ = nullptr;
```

`SkyrimEvents.cpp`（新增 sink 類別 + 在 install()/uninstall() 註冊，取代 §1 TODO）：

```cpp
class SkyrimEvents::MagicApplySink
    : public RE::BSTEventSink<RE::TESMagicEffectApplyEvent> {
public:
    MagicApplySink(EventSink* sink, SkyrimEntities* entities)
        : sink_(sink), entities_(entities) {}

    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESMagicEffectApplyEvent* a_event,
        RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) override {
        if (!a_event || !sink_ || !*sink_)
            return RE::BSEventNotifyControl::kContinue;

        // 1) 只接受玩家施法（避免 NPC 互施誤觸；事件量大，先擋）。
        auto* caster = a_event->caster.get();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!caster || !player || caster != player)
            return RE::BSEventNotifyControl::kContinue;

        // 2) 反查 target 的別名；非追蹤角色就不 fire（demo 只關心 "victim"）。
        auto* tgt = a_event->target.get();
        if (!tgt) return RE::BSEventNotifyControl::kContinue;
        std::string alias =
            entities_ ? entities_->aliasForFormID(tgt->GetFormID()) : "";
        if (alias.empty())
            return RE::BSEventNotifyControl::kContinue;

        // 3) 走既有 sink 路徑（adapter 會 marshal 回主執行緒 dispatchEvent）。
        nlohmann::json filter;
        filter["character"] = alias;
        (*sink_)("spell_cast_on", filter);
        return RE::BSEventNotifyControl::kContinue;
    }
private:
    EventSink* sink_;
    SkyrimEntities* entities_;
};

// 在 SkyrimEvents::install() 內，取代第 55–65 行的 TODO 區塊：
if (!magicApplySink_) {
    magicApplySink_ = new MagicApplySink(&sink_, &entities);
    holder->AddEventSink<RE::TESMagicEffectApplyEvent>(magicApplySink_);
    SKSE::log::info("SkyrimEvents: TESMagicEffectApplyEvent sink installed");
}

// 在 SkyrimEvents::uninstall() 內，鏡像 activateSink_ 的清理：
if (holder && magicApplySink_)
    holder->RemoveEventSink<RE::TESMagicEffectApplyEvent>(magicApplySink_);
delete magicApplySink_;
magicApplySink_ = nullptr;
```

要點：

- `filter["character"] = alias;` 的鍵名 **必須是 `character`**，才能對上 demo 觸發器
  （`QuestEngine.cpp:171` 逐鍵比對）。
- 不需要改 `SkyrimAdapter`：sink callback 早已在 `BuildEngine()`
  （`SkyrimAdapter.cpp:134`）綁定為 `FireEvent`，後者 marshal 到主執行緒
  （`SkyrimAdapter.cpp:230`）。
- 接好後，F10 debug 熱鍵（`SkyrimEvents.cpp:102`、`installDebugHotkeys`）即可移除或保留
  作為測試備援。
- `include`：`.cpp` 需引入
  `RE/T/TESMagicEffectApplyEvent.h`、`RE/P/PlayerCharacter.h`（或經 PCH 的
  `RE/Skyrim.h` 取得）。新增的成員 / 類別不需動 `cmake/sourcelist.cmake`
  （仍是同一個 `SkyrimEvents.cpp`）。

---

## 4. 已核對來源檔清單

- `CommonLibSSE-NG/include/RE/T/TESSpellCastEvent.h`（候選 A 簽章）
- `CommonLibSSE-NG/include/RE/T/TESHitEvent.h`（候選 B 簽章）
- `CommonLibSSE-NG/include/RE/T/TESMagicEffectApplyEvent.h`（候選 C 簽章）★
- `CommonLibSSE-NG/include/RE/T/TESActiveEffectApplyRemoveEvent.h`（候選 C' 簽章）
- `CommonLibSSE-NG/include/RE/C/CrosshairPickData.h` 與
  `build/.../vcpkg_installed/.../RE/C/CrosshairPickData.h`（候選 D；`target[]` 陣列差異）
- `CommonLibSSE-NG/include/RE/M/MagicCaster.h`、`RE/A/ActorMagicCaster.h`（候選 E）
- `CommonLibSSE-NG/include/RE/M/MagicTarget.h`、`RE/A/ActiveEffect.h`（候選 F / 反查細節）
- `CommonLibSSE-NG/include/RE/S/ScriptEventSourceHolder.h`（確認三個 holder 事件來源 +
  `AddEventSink<T>` API）
- `CommonLibSSE-NG/include/RE/B/BSTEvent.h`（`BSTEventSink::ProcessEvent` 簽章）
- `src/skyrim/SkyrimEvents.{h,cpp}`、`src/skyrim/SkyrimAdapter.{h,cpp}`、
  `src/skyrim/SkyrimEntities.{h,cpp}`、`src/core/QuestEngine.cpp`、
  `src/NpcGenerator.cpp`、`config/quests/demo_court_wizard.json`（接線脈絡）
