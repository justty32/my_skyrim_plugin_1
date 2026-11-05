# `spell_cast_on` 偵測：候選機制逐一核對

[返回總索引](spell_cast_on_hook.md)

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
