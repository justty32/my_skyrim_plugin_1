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

## 1. 候選機制逐一核對

### 候選 A：`RE::TESSpellCastEvent`（目前 NpcGenerator 已用）— ❌ 缺 target

Header：`CommonLibSSE-NG/include/RE/T/TESSpellCastEvent.h`

```cpp
struct TESSpellCastEvent
{
    NiPointer<TESObjectREFR> object;  // 00  ← 施法者（caster）
    FormID                   spell;   // 08  ← 法術 FormID
};
static_assert(sizeof(TESSpellCastEvent) == 0x10);
```

- 透過 `ScriptEventSourceHolder` 取得（`ScriptEventSourceHolder.h:106` 有
  `BSTEventSource<TESSpellCastEvent>` 基底；`AddEventSink<TESSpellCastEvent>` 可用）。
- 會對玩家施法觸發：✔（`NpcGenerator.cpp` 的 `SpellCastHandler` 就是靠它偵測玩家自訂法術）。
- 取得 caster：✔（`object`）。取得 spell：✔（`spell`）。
- **取得 target：✘**。此事件只在「施法動作發生」當下送出，**完全沒有目標欄位**，
  且在投射物 / 觸碰命中之前就觸發，根本還不知道會打到誰。
- `SkyrimEvents.cpp:55-65` 的 TODO 註解對此判斷正確：「TESSpellCastEvent 只報
  CASTER 與 spell，不報法術落在誰身上」。
- 結論：**單獨不可用**。除非搭配候選 D（讀 crosshair）湊出 target，見 §2。

### 候選 B：`RE::TESHitEvent` — △ 有 target，但語意是「命中傷害」不是「施法」

Header：`CommonLibSSE-NG/include/RE/T/TESHitEvent.h`

```cpp
struct TESHitEvent
{
    enum class Flag { kNone, kPowerAttack, kSneakAttack, kBashAttack, kHitBlocked };
    NiPointer<TESObjectREFR> target;      // 00  ← 被命中者 ✔
    NiPointer<TESObjectREFR> cause;       // 08  ← 攻擊來源（caster/aggressor）✔
    FormID                   source;      // 10  ← 武器/法術 base form
    FormID                   projectile;  // 14
    stl::enumeration<Flag, std::uint8_t> flags;  // 18
    ...
};
static_assert(sizeof(TESHitEvent) == 0x20);
```

- 透過 `ScriptEventSourceHolder` 取得（`ScriptEventSourceHolder.h:82`）。
- 會對玩家攻擊觸發：✔；**同時帶 `target` 與 `cause`**：✔ —— 這是它的優點。
- 取得「法術」：`source` 是命中來源的 base form FormID。對法術而言常是
  **MagicItem / spell** 的 FormID（可 `LookupByID` 後 `As<RE::SpellItem>()` 判斷），
  但 header 沒有任何欄位語意保證它一定是 spell，所以要過濾「是法術」需自行判型。
- **語意風險（關鍵）**：`TESHitEvent` 是「造成命中 / 傷害」事件。
  - 只施加**增益 / 非傷害效果**的法術（解詛咒、治療、buff 等）**不一定觸發 hit**。
    本案 `lift_curse` 是「對侍從施法解咒」，極可能是非敵對 / 無傷害的法術，
    這類法術很可能根本不產生 `TESHitEvent`。
  - 對 self / touch / 持續型法術的觸發行為不一致。
- 結論：**對「敵對 / 傷害型法術命中某目標」很合適，但對「解咒 / buff 型施法」不可靠**。
  不建議作為 `spell_cast_on` 的主要來源。

### 候選 C：`RE::TESMagicEffectApplyEvent` — ✔✔ 同時有 caster + target + 效果，且涵蓋非傷害法術

Header：`CommonLibSSE-NG/include/RE/T/TESMagicEffectApplyEvent.h`

```cpp
struct TESMagicEffectApplyEvent
{
    NiPointer<TESObjectREFR> target;       // 00  ← 法術效果施加的對象 ✔✔
    NiPointer<TESObjectREFR> caster;       // 08  ← 施法者 ✔✔
    FormID                   magicEffect;  // 10  ← 被施加的 EffectSetting(MGEF) FormID ✔
    std::uint32_t            pad14;        // 14
};
static_assert(sizeof(TESMagicEffectApplyEvent) == 0x18);
```

- 透過 `ScriptEventSourceHolder` 取得：✔，holder 確實繼承
  `BSTEventSource<TESMagicEffectApplyEvent>`（`ScriptEventSourceHolder.h:86`）。
  以 `holder->AddEventSink<RE::TESMagicEffectApplyEvent>(sink)` 安裝，與既有
  `ActivateSink` 完全相同的安裝方式（`SkyrimEvents.cpp:51`）。
- 會對玩家施法觸發：✔（玩家是 `caster` 時照樣送出）。
- **同時帶 `caster` 與 `target`**：✔✔ —— 正是 `spell_cast_on` 需要的兩端。
- 涵蓋非傷害法術：✔ —— 只要法術的某個 magic effect 被施加到目標上就觸發，
  解咒 / buff / 治療都會送出（與 `TESHitEvent` 的傷害語意不同），正好對應 demo 的
  「對侍從施法解咒」。
- 取得「法術」：`magicEffect` 是 **MGEF（EffectSetting）** 的 FormID，**不是 SpellItem**。
  若日後要過濾「哪一個法術」，需以 MGEF 或法術上的某個 keyword 過濾，而非 spell FormID
  直接比對。本案 demo 不過濾法術，故此限制不影響。
- 細節注意：一個法術可能含多個 magic effect，**同一次施法會對同一 target 送出多個
  `TESMagicEffectApplyEvent`**（每個 effect 一次）。因此可能 fire 多次。對本案無害
  （`complete_objective` 重複呼叫是冪等的：objective 一旦完成，後續 `when` 的
  `objective_state == active` 條件就不再成立，`QuestEngine.cpp:191` 的 `when` 檢查會擋掉），
  但若日後改接會「+1 計數」的動作，需要自行去重。
- 結論：**對 `spell_cast_on` 最貼切的純事件來源**。零手動 hook、零 RELOCATION_ID。

### 候選 C'：`RE::TESActiveEffectApplyRemoveEvent` — ✔ 同樣有 caster+target，但多了 add/remove 與 ID 細節

Header：`CommonLibSSE-NG/include/RE/T/TESActiveEffectApplyRemoveEvent.h`

```cpp
struct TESActiveEffectApplyRemoveEvent
{
    NiPointer<TESObjectREFR> caster;                // 00  ✔
    NiPointer<TESObjectREFR> target;                // 08  ✔
    std::uint16_t            activeEffectUniqueID;   // 10  ← 對應 ActiveEffect::usUniqueID
    bool                     isApplied;              // 12  ← true=施加, false=移除
    ...
};
static_assert(sizeof(TESActiveEffectApplyRemoveEvent) == 0x18);
```

- holder 也有此來源（`ScriptEventSourceHolder.h:67`，安裝方式同上）。
- 與候選 C 幾乎等價，差別：
  - 多一個 `isApplied`，可區分「施加 vs 移除」（用 `isApplied` 過濾掉移除事件）。
  - 給的是 `activeEffectUniqueID`（要拿到 spell 需再經 `target` 的
    `MagicTarget`/`Actor` 去 `GetActiveEffectList()` 比對 `usUniqueID`，較繁瑣）；
    而候選 C 直接給 `magicEffect` FormID，取效果更直接。
- 結論：可用，**但若只是要 (caster,target)，候選 C 更簡單**。若日後需要「效果被移除」
  的語意（例如偵測詛咒被解除而非被施加），此事件才有獨到價值。

### 候選 D：`RE::CrosshairPickData`（在 `TESSpellCastEvent` 內讀準星）— △ 可湊出 target 但不精確

Header（**以實際編譯用的 vcpkg 樹為準**）：
`build/release-clang-cl-linux/vcpkg_installed/.../RE/C/CrosshairPickData.h`

```cpp
class CrosshairPickData {
public:
    static CrosshairPickData* GetSingleton() {
        static REL::Relocation<CrosshairPickData**> singleton{ RELOCATION_ID(515446, 401585) };
        return *singleton;
    }
    // ... 非 EXCLUSIVE_SKYRIM_FLAT 組態下（本專案 CommonLibSSE-NG-fork 多版本通用）：
    ObjectRefHandle target[VR_DEVICE::kTotal];       // 04  ← 注意是「陣列」
    ObjectRefHandle targetActor[VR_DEVICE::kTotal];  // 10
    ...
};
```

> ⚠ header 差異糾正：規範樹（`CommonLibSSE-NG/include/.../CrosshairPickData.h`）寫的是
> 單一 `ObjectRefHandle target;`（`EXCLUSIVE_SKYRIM_FLAT` 變體）。但 `NpcGenerator.cpp:24`
> 寫 `crosshair->target[0]` 之所以能編譯，是因為**實際編譯用的 vcpkg 樹**在非 FLAT
> 組態下把 `target` 宣告為 `target[VR_DEVICE::kTotal]` 陣列（VR 相容）。
> 引用此 API 時請以 vcpkg 樹為準，並維持 `target[0]`（= 主視角）的寫法。

- 機制：在候選 A 的 `TESSpellCastEvent` 回呼裡，於施法瞬間讀
  `CrosshairPickData::GetSingleton()->target[0].get()` 當作「victim」。
  `NpcGenerator.cpp:21 GetCrosshairTarget()` 已是這個模式。
- 取得 target：△ —— 取得的是「施法當下準星指著的東西」，**不是法術真正命中的對象**：
  - 投射物在空中時準星已移開、AoE、自我增益、觸碰未命中等情形都會錯位。
  - 玩家沒瞄準任何 ref 時 `target[0]` 為空。
- 結論：可作為**降級備援**（在沒有更好來源時），但精確度遜於候選 C。不建議作主路。

### 候選 E：`MagicCaster` / `ActorMagicCaster` 虛擬函式 hook — ✘（需手動 offset，過度工程）

Header：`CommonLibSSE-NG/include/RE/M/MagicCaster.h`、`RE/A/ActorMagicCaster.h`

相關簽章（已核對）：

```cpp
// MagicCaster.h
virtual void CastSpellImmediate(MagicItem* a_spell, bool a_noHitEffectArt,
        TESObjectREFR* a_target, float a_effectiveness, bool a_hostileEffectivenessOnly,
        float a_magnitudeOverride, Actor* a_blameActor);          // vfunc 01 —— 帶 spell + target ✔
bool FindTargets(float, std::uint32_t& a_targetCount, TESBoundObject* a_source,
        bool a_loadCast, bool a_adjustOnlyHostileEffectiveness);   // 非虛擬成員函式
MagicItem* currentSpell;          // 偏移 0x28
ObjectRefHandle desiredTarget;    // 偏移 0x20

// ActorMagicCaster.h（繼承 MagicCaster + SimpleAnimationGraphManagerHolder + BSTEventSink<BSAnimationGraphEvent>）
Actor* actor;                     // 偏移 0xB8 —— 施法者本人
```

- 理論上 hook `CastSpellImmediate`（vtable 槽 01）可同時拿到 `a_spell` 與 `a_target`，
  資訊最完整。`ActorMagicCaster::actor`（0xB8）給施法者。
- **但這是 vtable / trampoline hook**：
  - 需自行找到並驗證每個遊戲版本（SE/AE/GOG/VR）的 vtable 位址或 RELOCATION_ID。
    **TODO：address-library offset 必須逐版本手動尋找，本文件不杜撰任何 ID**（與
    CLAUDE.md「Hook IDs / offsets 仍需手動尋找」一致）。
  - 風險高：誤判 vtable 槽位 / CRT 與 ABI 細節 / 與其他 mod 衝突。
  - 對「只要知道 (caster,target)」的需求屬**過度工程**。
- 結論：**不建議**。除非未來需要在「施法瞬間 / 命中前」就介入或拿到 SpellItem 等
  事件來源拿不到的資訊，才值得承擔手動 hook 成本。

### 候選 F：`MagicTarget::MagicTargetHit` / `MagicTarget::AddTarget` hook — ✘（同 E，需手動 offset）

Header：`CommonLibSSE-NG/include/RE/M/MagicTarget.h`

```cpp
struct AddTargetData {
    TESObjectREFR* caster;     // 00  ✔
    MagicItem*     magicItem;  // 08  ✔（這裡是真正的 spell！）
    Effect*        effect;     // 10
    ...
};
virtual bool AddTarget(AddTargetData& a_targetData);  // vfunc 01 —— this 即 target
```

- `MagicTarget::AddTarget`（vtable 槽 01）在「效果即將施加到此 target」時被呼叫：
  `this` 是 target，`a_targetData.caster` 是施法者，`a_targetData.magicItem` 是
  **真正的 MagicItem/SpellItem**（比候選 C 的 MGEF FormID 更接近「法術」）。
- 注意：header 沒有名為 `MagicTargetHit` 的成員（任務描述中的名稱在此版本 header
  並不存在）；最接近的注入點是 `AddTarget`。
- 同候選 E：**vtable hook，需手動逐版本 offset，本文件不杜撰 ID（TODO）**。風險高。
- 結論：**不建議**作為主路；列為「需要精確 SpellItem 過濾時」的進階備援。

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
