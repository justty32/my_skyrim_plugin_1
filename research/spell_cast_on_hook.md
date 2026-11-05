# `spell_cast_on` 偵測機制可行性分析（header 已核對）

> 目標：在執行期偵測「玩家對某個目標 Y 施放了法術 X」，以實作 quest engine 的
> `spell_cast_on` 觸發器。目前該觸發器由 debug F10 熱鍵（`SkyrimEvents::DebugInputSink`）
> 偽造，我們要的是真正的偵測。
>
> 本文件所有類別 / 成員 / 事件簽章皆已對照 CommonLibSSE-NG 實際 header 核對，未憑記憶。
> 主要核對來源（規範樹）：
> `/home/lorkhan/repo/my_skyrim_plugin_1/CommonLibSSE-NG/include/RE/...`
> 以及實際編譯所用的 vcpkg 樹（`target[]` 陣列差異關鍵）：
> `/home/lorkhan/repo/my_skyrim_plugin_1/build/release-clang-cl-linux/vcpkg_installed/x64-windows-skse-clang/include/RE/...`

---

## 0. 現有觸發器要什麼（決定了「足夠好」的門檻）

來自 `config/quests/demo_court_wizard.json`（第 22–24 行）：

```json
{ "on": "spell_cast_on", "character": "victim",
  "when": { "objective_state": { "objective": "lift_curse", "state": "active" } },
  "do": [ { "complete_objective": "lift_curse" } ] }
```

觸發器比對邏輯在 `src/core/QuestEngine.cpp:162` `triggerMatches()`：除了結構鍵
`on/when/do` 之外，**觸發器上的每個 filter 鍵都必須等於事件 filter 對應的值**。
本案的 demo 觸發器只有一個 filter 鍵 `character`，值為別名字串 `"victim"`。

關鍵推論：

- **本案 demo 不需要過濾「哪一個法術」**——任何法術打到 `victim` 都算過關。
- 因此核心需求縮減為：**取得「被法術命中的目標」並反查回它的 JSON 別名**。
- adapter 端已有現成的反查工具 `SkyrimEntities::aliasForFormID(FormID)`
  （`src/skyrim/SkyrimEntities.h:45`，實作於 `SkyrimEntities.cpp:69`，遍歷
  `resolved_` 內已綁定 / 已 spawn 的 ActorHandle 比對 FormID）。`victim` 是由對話
  動作 `spawn_character` 生成、登記進 `resolved_` 的，所以一旦拿到目標的 FormID
  就能反查出 `"victim"`。這與既有的 `ActivateSink` 走的是同一條路（`SkyrimEvents.cpp:22`）。

> 換言之：只要找到一個「會對玩家施法、且事件本身帶有 target ref」的事件來源，
> 整條線就能用既有零件接起來，無需任何手動 hook / RELOCATION_ID。

---

## 2. 比較總表

| 機制 | 取得方式 | caster | target | 法術資訊 | 對玩家觸發 | 涵蓋非傷害法術 | 需手動 offset | 風險 |
|---|---|---|---|---|---|---|---|---|
| A `TESSpellCastEvent` | holder 事件 | ✔ object | ✘ | ✔ spell FormID | ✔ | n/a | 否 | 低，但**無 target 不可用** |
| B `TESHitEvent` | holder 事件 | ✔ cause | ✔ target | △ source FormID | ✔ | ✘（傷害語意） | 否 | 低，但解咒型法術不觸發 |
| **C `TESMagicEffectApplyEvent`** | **holder 事件** | **✔ caster** | **✔ target** | △ MGEF FormID | ✔ | **✔** | **否** | **低** |
| C' `TESActiveEffectApplyRemoveEvent` | holder 事件 | ✔ caster | ✔ target | △ activeEffectUniqueID | ✔ | ✔ | 否 | 低 |
| D crosshair（搭 A） | RELOCATION 單例 | ✔ | △ 準星近似 | ✔ spell | ✔ | ✔ | 否(ID 已在 CLib) | 中，不精確 |
| E `(Actor)MagicCaster` hook | vtable hook | ✔ | ✔ | ✔ SpellItem | ✔ | ✔ | **是 (TODO)** | 高 |
| F `MagicTarget::AddTarget` hook | vtable hook | ✔ | ✔ | ✔ MagicItem | ✔ | ✔ | **是 (TODO)** | 高 |

---

## 3. 建議（RECOMMENDATION）

**採用候選 C：`RE::TESMagicEffectApplyEvent` 的 `BSTEventSink`，經
`ScriptEventSourceHolder` 安裝。風險等級：低。**

理由：

1. **它是 `spell_cast_on` 語意上最貼切的純事件來源**：事件本身同時帶 `caster` 與
   `target`，且對「解咒 / buff / 治療」這類非傷害法術也會送出（候選 B 不會），
   正好對應 demo 的「對受詛咒的侍從施法解咒」。
2. **零手動 hook、零 RELOCATION_ID**：與既有 `ActivateSink` 用同一套
   `holder->AddEventSink<T>(sink)` 機制（`SkyrimEvents.cpp:51`），跨 SE/AE/GOG/VR
   自動相容，無 PITFALLS.md 列的 hook 風險。
3. **與現有零件無縫接合**：拿到 `target->GetFormID()` 後直接餵 `aliasForFormID()`
   反查別名，沿用 `ActivateSink` 已驗證的 fire 路徑（sink → `FireEvent` →
   主執行緒 `dispatchEvent`）。
4. demo 觸發器**不過濾法術**，所以候選 C 只給 MGEF FormID（而非 SpellItem）的限制
   不影響本案；可加一道「只接受 caster 是玩家」的過濾以避免 NPC 互相施法誤觸。

備援：若日後 `spell_cast_on` 需精確過濾「哪一個 SpellItem」，再評估升級到候選 F
（`MagicTarget::AddTarget`，可拿到 `MagicItem*`）；屆時須補上逐版本 address-library
offset（**TODO，本文件不提供 ID**）。候選 D（crosshair）僅在需要近似 target 時作降級。

> 開銷提醒：`TESMagicEffectApplyEvent` 在遊戲中相當頻繁（任何 NPC 任何效果施加都送），
> 故 sink 內**第一步就過濾**（caster 是否為玩家、target 是否反查得到別名），不命中即
> 立刻 `return kContinue`，避免每幀大量 JSON 構造。

---

本分析已按主題拆分：

- [候選機制逐一核對](spell_cast_on_hook_candidates.md)：候選 A–F（事件 sink 與 vtable hook）逐一對照 header 的取得方式與限制。
- [最小接線草圖與核對來源](spell_cast_on_hook_wiring.md)：接進 `SkyrimEvents` 的最小 sink 草圖與已核對來源檔清單。
