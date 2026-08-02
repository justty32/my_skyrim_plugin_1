# 執行期程序化生成 NPC（Form 層級）— SKSE C++ 可行性與設計分析

> 探索性可行性分析，2026-05-23。由 research agent 產出。所有引擎面論斷均對照本 repo vendored 的
> `CommonLibSSE-NG/include/RE/` 標頭逐一查證並標出真實路徑/行號。**僅分析，未動 plugin 程式碼**
> （`src/`、`cmake/`、`plugin.cpp` 一律不碰）。
> 與本專案 ethos 一致（`CLAUDE.md`、`MODDING_COOKBOOK.md`、memory `project_quest_engine`）：
> **不開 Creation Kit、不做 ESP/ESL，全部在執行期用程式碼。**
> 交叉引用：`research/ALCHEMY_SPIKE_FINDINGS.md`（動態 `AlchemyItem` 的**同一族**持久化問題）、
> `research/NPC_BEHAVIOR_DEEP_MODIFICATION.md`（actor 存在後的 AI/package）、
> `research/PROCGEN_INTERIOR.md`（「存種子+食譜、讀檔重建」hybrid 模式的先例）。

---

## 核心問題（使用者的原話）

> SKSE plugin 能否在執行期**在 Form 層級程序化生成一個 NPC**——一個真正的、能存進存檔的
> `TESNPC_` / `TESActorBase` form，由 C++ 鑄造出來，而**不只是把既有 base form 暫時擺一個 reference**？

使用者明確對比「**暫時的**」（temporary placed ref）vs「**Form**」（真正的 base form）。整份報告的成敗在兩件事：
**(a)** 能不能在執行期鑄造一個新的 `TESNPC_` form？
**(b)** 它能不能正確跨 save/reload 存活、且保有穩定身分（像 ESP 作者寫的 NPC 那樣）？

社群普遍認為「沒有 ESP/ESL 就做不到持久的 NPC」。以下逐點查證，並解釋**為什麼**。

---

## 結論先講（TL;DR）

| 問題 | 結論 |
|---|---|
| (a) 能否執行期鑄造一個 `TESNPC_` form？ | **能**。`IFormFactory::GetConcreteFormFactoryByType<TESNPC>()` 存在且可用；本 repo `NpcGenerator::SpawnNpc` **已經這樣做**。鑄出來的是真 form，拿到 `0xFF......` 動態 FormID，當下 session 完全可用。 |
| (a') 能否複製既有 base 再改？ | **能，且更務實**。`TESForm::CreateDuplicateForm` 對 `TESNPC` 並未被覆寫成 `{return 0;}`（不像 `CELL`），可走預設實作；或更穩的 `factory->Create()` + `newBase->Copy(template)`（本 repo 現行做法）。複製品同樣得到 `0xFF......` FormID。 |
| (b) 它能否跨 save/reload 持久、保有穩定身分？ | **不能可靠做到**。這是硬限制，理由見 §3。動態 form 的持久化由引擎 codec 決定：`TESNPC`（FormType::NPC）**不在會被序列化的型別清單裡**；即使是會被序列化的型別（WEAP/ARMO/MISC），重載後也**只剩 form type、資料全空**（無名、無模型）。SKSE 的 `SerializationInterface` 救不了「鑄造一個全新 base form」這件事（只能存 blob、remap 既有 plugin form 的 FormID）。 |
| 務實建議 | **co-save 食譜 + 讀檔重建** hybrid（§5），與 `PROCGEN_INTERIOR.md` §5.2 策略 B、`QUEST_ENGINE_DESIGN.md` 的 spawned 失效政策完全同一條路。或退一步：**複用/改寫既有 vanilla base**（§6）。 |

**一句話**：能造、能用一個 session；**不能讓「我發明的全新 base form」像 ESP 作者的 NPC 那樣自己活過存檔**。要持久，必須自己存食譜、讀檔重造。

---

## 0. 本 repo 已有的地基（這題其實一半已經做了）

`src/NpcGenerator.cpp` 的 `SpawnNpc()`（行 60–99）**已經實作了「從工廠造 `TESNPC_` base form」這條路**，不是只擺既有 base 的 ref。逐行拆解：

```cpp
// NpcGenerator.cpp:63-78
auto* templateNPC = RE::TESForm::LookupByID<RE::TESNPC>(0x00000007);          // 玩家 NPC 當模板
auto* factory     = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>();
auto* newNpcBase  = factory->Create()->As<RE::TESNPC>();                      // ← 鑄一個全新 TESNPC base form
newNpcBase->Copy(templateNPC);                                               // ← 從模板拷貝全部資料
newNpcBase->fullName = "Generated Citizen";                                  // ← 改名
auto spawned = a_anchor->PlaceObjectAtMe(newNpcBase, false);                 // ← 用這個新 base 擺一個 ref（Actor）
```

**這正是使用者要問的「Form 層級生成」**——`newNpcBase` 是一個真正的 `TESNPC` form（拿到 `0xFF......` 動態 FormID），不是「擺既有 base 的暫時 ref」。`PlaceObjectAtMe` 才是「擺 ref」，而它擺的是**剛鑄出來的新 base**。

對照 `NpcGenerator` 裡的另外兩種行為，差別很清楚：
- **擺既有 base 的暫時 ref**（使用者說的「暫時的」）：`PlaceTree()`（行 154–191）`LookupByEditorID<TESBoundObject>("TreeFloraJuniper01")` → `PlaceObjectAtMe`；`SpawnAtLocation()`（行 30–56）`CreateReferenceAtLocation(...)`。base 是既有的，只是多了一個 ref。
- **鑄新 base form**（使用者說的「Form」）：`SpawnNpc()` 的 `factory->Create()`，以及 `InitializeMagic()`（行 268、282）對 `EffectSetting`/`SpellItem` 用 `factory->Create()`——這是同一個工廠模式，**且本 repo 已驗證能編譯/連結並在 session 內運作**（dynamic spell 在遊戲裡可施放）。

> ⚠️ **重點澄清**：`IFormFactory` 工廠模式確實能延伸到 `TESNPC_`（`ConcreteFormFactory_TESNPC` 的 RTTI/VTABLE 在 `RE/Offsets_VTABLE.h:926` 確實存在），`SpawnNpc` 就是證據。問題從來不在「能不能造」，而在「造出來的東西活不活過存檔」——這點 `NpcGenerator` **沒有處理**（它和 dynamic spell 一樣，靠 `kNewGame`/`kPostLoadGame` 重給，見 `MODDING_COOKBOOK.md:53`）。本報告補上的就是這塊。

---

## 1. 從零鑄造一個 `TESNPC_`

### 1.1 型別與工廠（已查證）

- `class TESNPC : public TESActorBase, public TESRaceForm, public BGSOverridePackCollection, public BSTEventSink<MenuOpenCloseEvent>`（`T/TESNPC.h:31-36`），`FORMTYPE = FormType::NPC`（行 42），`sizeof == 0x268`（行 302）。
- `class TESActorBase : public TESBoundAnimObject, TESActorBaseData, TESContainer, TESSpellList, TESAIForm, TESFullName, ActorValueOwner, BGSDestructibleObjectForm, BGSSkinForm, BGSKeywordForm, BGSAttackDataForm, BGSPerkRankArray`（`T/TESActorBase.h:19-31`）——一個「角色 base」是這 12 個元件的合體。
- 工廠：`IFormFactory::GetConcreteFormFactoryByType<RE::TESNPC>()`（`I/IFormFactory.h:40`，`C/ConcreteFormFactory.h`）→ `Create()`（`IFormFactory.cpp:23` → `CreateImpl()`，型別專屬的 `ConcreteFormFactory<TESNPC, FormType::NPC>::CreateImpl`）。`CreateImpl` 由引擎實作，會 **`new` 一個 `TESNPC`、配一個動態 FormID、掛進 form 表**。`ConcreteFormFactory_TESNPC` 在 `RE/Offsets_VTABLE.h:926` 確認存在。

### 1.2 一個剛出爐的 `TESNPC_` 缺什麼

`factory->Create()` 給的是一個**幾乎全空**的 `TESNPC`。對照 `T/TESNPC.h` / `T/TESActorBaseData.h` 的成員，要它變成「能在遊戲裡正常顯示與運作的 NPC」，至少要補：

| 缺的東西 | 成員 / 出處 | 不補的後果 |
|---|---|---|
| **種族（race）** | `TESRaceForm`（`TESNPC` 繼承，含 `GetRace()` 行 250）；`originalRace`（行 274） | 無 race = 無骨架/動畫/體格 → 不會渲染或 crash |
| **臉/頭件/上色** | `headParts`/`numHeadParts`（行 285-286）、`faceData`（行 296）、`tintLayers`（行 297）、`bodyTintColor`（行 292）、`headRelatedData`（髮色/臉貼圖，行 269） | 無臉 = 空白頭或 default 臉（§4） |
| **性別** | `ACTOR_BASE_DATA::Flag::kFemale`（`TESActorBaseData.h:22`） | 預設男性，與外觀不符 |
| **技能/屬性** | `Skills playerSkills`（行 267，含 health/magicka/stamina 行 105-107）；`ACTOR_BASE_DATA` 的 level/offset（`TESActorBaseData.h:66-76`） | 0 血/0 技能 → 一打就死或行為怪 |
| **職業** | `npcClass`（行 268，`TESClass*`） | 無職業 = AI/技能成長無依據 |
| **派系** | `TESActorBaseData::factions`（`TESActorBaseData.h:130`）；`crimeFaction`（行 284） | 無派系 = 中立/孤立，無法接 quest/犯罪系統 |
| **AI 資料** | `TESAIForm`（`TESNPC` 繼承）；`defaultPackList`（行 283，`BGSListForm*`） | 無包 = 站著不動（見 `NPC_BEHAVIOR_DEEP_MODIFICATION.md` §1） |
| **語音** | `TESActorBaseData::voiceType`（`TESActorBaseData.h:127`）；`GetObjectVoiceType()`（`TESNPC.h:221`） | 無語音 = 不能對話、無戰鬥吼聲 |
| **庫存/裝備** | `TESContainer`（`TESNPC` 繼承）；`defaultOutfit`/`sleepOutfit`（行 281-282，`BGSOutfit*`） | 裸體、無武器 |
| **戰鬥風格** | `combatStyle`（行 271，`TESCombatStyle*`） | 戰鬥行為退化 |

**這就是為什麼從零造 NPC 很痛**：要手動填的東西太多，且很多是互相關聯的指標（race↔skeleton↔headparts、class↔skills、faction↔relationship）。

### 1.3 最佳實務：別從零，從模板 `Copy`

本 repo `SpawnNpc` 已選對路：`newNpcBase->Copy(templateNPC)`（`NpcGenerator.cpp:75`）。`Copy` 是 `TESNPC` override 的 vfunc 2F（`TESNPC.h:217`），會把模板 NPC 的**所有上述元件一次拷貝**過來，得到一個「立刻能用的完整 NPC」，再針對性覆寫想改的欄位（名字、性別、外觀、派系…）。
- ⚠️ 模板用 `0x7`（玩家 base）只是占位（`COURT_WIZARD_DESIGN.md:192` 也標占位）；正式應挑一個合適的 vanilla NPC base（如某個 generic citizen / bandit）當模板，省下補 race/class/voice 的工。

---

## 2. 複製既有 base：`CreateDuplicateForm`

- `virtual TESForm* CreateDuplicateForm(bool a_createEditorID, void* a_arg2)`（`T/TESForm.h:132`，vfunc 09）。
- **關鍵差異**：`PROCGEN_INTERIOR.md` §1 發現 `TESObjectCELL` 把它覆寫成 `{ return 0; }`（`T/TESObjectCELL.h:181`，註解寫明）。**但 `TESNPC` 並未覆寫 `CreateDuplicateForm`**——grep 全 vendored 標頭，覆寫它的只有 `TESObjectLAND` / `TESIdleForm` / `TESWorldSpace` / `TESObjectCELL` / `TESObjectREFR` / `BGSCameraPath`（皆 `T/*.h` 內列出），**`TESNPC.h` 沒有這行**。所以 `TESNPC` 走 `TESForm` 的**預設** `CreateDuplicateForm`，理論上會回一個非 0 的複製品（與 CELL 的明確封死相反）。
- 複製品同樣是動態 form，拿 `0xFF......` FormID（`TESForm::IsDynamicForm()` = `GetFormID() >= 0xFF000000`，`T/TESForm.h:326`）。

**`CreateDuplicateForm` vs `factory->Create()+Copy()` 哪個好？**
- 兩者都得到動態 FormID 的新 base，**持久化命運完全相同**（§3）——選哪個不改變「活不活過存檔」的結論。
- `factory->Create()+Copy()` 是**本 repo 已驗證、已在用**的路（`SpawnNpc`），且 `Copy` 語意明確、可控。**建議沿用它**，不必為了 `CreateDuplicateForm` 冒「`a_arg2` 未知參數 / 預設實作對 NPC 的細節未驗證」的風險。
- `CreateDuplicateForm` 的 `a_arg2` 在標頭裡是 `void*`、語意不明，且預設實作對 `TESNPC` 的行為（會不會正確深拷 headParts/factions 陣列指標）**未經本 repo 實測**——列為次要選項。

---

## 3. 動態 FormID 與持久化的核心難題（本報告的重點）

### 3.1 動態 FormID 怎麼配（已查證）

- 執行期造的 form 落在**動態範圍 `0xFF000000+`**（`TESForm::IsDynamicForm()`：`GetFormID() >= 0xFF000000`，`T/TESForm.h:326`）。這是 load order 裡保留給「遊戲執行中動態產生」的 mod index（對應社群說的「FF 前綴」）。
- 配號器是 `TESDataHandler::nextID`（`T/TESDataHandler.h:175`，offset 0xD50）。工廠 `CreateImpl` 內部從這裡取下一個動態 FormID 並掛進 `TESDataHandler::formArrays[FormType::NPC]`（行 167）。`AddFormToDataHandler(TESForm*)`（行 43）是把 form 掛進表的入口。
- **關鍵性質**：`nextID` 是**執行期遞增的計數器**。它**不保證跨 save/reload 給同一個邏輯物件同一個號**——重載後 `nextID` 從另一個起點走，你上次拿到的 `0xFF00ABCD` 這個 session 可能根本不存在，或被配給完全不同的東西。**動態 FormID 本質上是 session-local 的，不是穩定身分。**

### 3.2 引擎 codec 會不會序列化一個執行期造的 `TESNPC_`？——不會（可靠地）

這是整份報告的命門。社群逆向 + 框架作者文件給出明確答案（見 §7 web 查證）：

1. **大多數 form type 根本不被存檔序列化。** 引擎只對**特定型別**的動態 form 寫進存檔——社群實證點名的是 **WEAP / ARMO / MISC**（武器/盔甲/雜物，因為玩家附魔產生的動態 enchanted item 必須存）。**`FormType::NPC` 不在此列。** 一個執行期造的 `TESNPC` 重載後**直接消失**（form 不存在，FormID 變懸空）。
2. **即使是「會被序列化」的型別，重載後也只剩骨架。** 社群實證：被序列化的動態 form「load game 之初**除了 form type 以外沒有任何資料**——空名、無模型、0 重量」。掛在上面的 script 還在（因為 script 由 SKSE/papyrus VM 另存），但**form 本體的資料全空**。換成 NPC 的語境：就算 NPC 真被序列化，重載後也會是「無 race、無臉、無 class、無 faction 的空殼」——等於沒救活。
3. **dangling reference（懸空參照）**：你用 `PlaceObjectAtMe(newNpcBase, ...)` 擺的那個 actor ref，其 `baseObject` 指向這個動態 base。base 重載後不存在 → ref 的 base 懸空 → 該 actor ref 不是消失就是壞掉（無模型/crash）。任何指向這個 base 的 quest alias、relationship（`TESNPC::relationships`，`T/TESNPC.h:295`）一併斷裂。

> **這與 `ALCHEMY_SPIKE_FINDINGS.md` 是同一族問題。** 該 spike 對動態 `AlchemyItem`（FormType::AlchemyItem，也不在 WEAP/ARMO/MISC 清單）手動 insert 進 `BGSCreatedObjectManager::potions`，並把「**存檔 codec 會不會序列化這個 dynamic form**」列為**第一風險**（`ALCHEMY_SPIKE_FINDINGS.md` 行 32、120-124、§4 Test B）。NPC 的處境**更糟**：potion 還有 `BGSCreatedObjectManager` 這個「玩家自製物」的官方管理器當掛勾候選；**NPC base 沒有對應的 `CreatedObjectManager`**，連「插進某個官方 bank 讓 codec 看到」的候選都沒有。所以 alchemy spike 的「conditional GO（待實測）」對 NPC 應讀作「**default NO**」。

### 3.3 `SKSE::SerializationInterface` 能做什麼、不能做什麼（已查證）

`SKSE::SerializationInterface`（`SKSE/Interfaces.h:81`）：

- ✅ **能**：用 `OpenRecord(type, version)`（行 125）/ `WriteRecord`（行 101）/ `WriteRecordData`（行 127）/ `ReadRecordData`（行 153）把**任意 byte blob**寫進 .skse co-save，並用 `SetSaveCallback`/`SetLoadCallback`/`SetRevertCallback`（行 97-99）掛回呼。這就是「存一份食譜」的機制。
- ✅ **能**：`ResolveFormID(oldFormID, newFormID)`（行 177）——把**存進你 blob 裡的、屬於既有 plugin 的 FormID**（如 `0x000XXXXX~Skyrim.esm`）remap 成本次 load order 下的正確 FormID。`ResolveHandle`（行 178）同理對 handle。
- ❌ **不能**：`ResolveFormID` **不會、也不能「復活一個你發明的全新 base form」**。它只重映既有 form 的號；對一個重載後根本不存在的動態 `TESNPC`，`ResolveFormID` 回 false（解析失敗），給不出任何東西。
- ❌ **不能**：把一個 C++ 物件（`TESNPC*`）原樣塞進存檔讓引擎下次幫你重建——co-save 只認 byte blob，重建邏輯得你自己寫。

**所以 SKSE serialization 對本題的角色是**：存「重建這個 NPC 所需的食譜」（用哪個模板、改了哪些欄位、外觀參數、放在哪），讀檔時**由你自己重跑 §1 的造 NPC 流程**。它**不是**「讓引擎幫你存一個新 base form」的萬靈丹。

---

## 4. 外觀程序化生成（拿到 `TESNPC_` 之後）

假設已有一個 `TESNPC`（造的或複製的），怎麼程序化設外觀？

**可在 base form 上設定的資料（`T/TESNPC.h`）**：
- **頭件**：`headParts`（行 285，`BGSHeadPart**`）/ `numHeadParts`（行 286）；逐型別 `BGSHeadPart::HeadPartType { kMisc=0, kFace=1, kEyes=2, kHair=3, ... }`（`B/BGSHeadPart.h:33-38`）。helper：`TESNPC::ChangeHeadPart`（行 240）、`GetHeadPartByType`（行 244）、`GetCurrentHeadPartByType`（行 243）。
- **臉型 morph / 預設**：`faceData`（行 296，`FaceData*`），含 19 個 morph（`FaceData::Morphs`，行 137-162：鼻/顎/頰/眼/眉/唇/下巴…）與 4 個 part 預設（`Parts`，行 165-176）。
- **上色（tint）**：`tintLayers`（行 297，`BSTArray<Layer*>*`），`Layer = { tintColor, tintIndex, preset, interpolationValue }`（行 190-203）；`bodyTintColor`（行 292）；皮膚色 `SetSkinFromTint(NiColorA*, TintMask*, bool)`（行 262）。`TintMask`（`T/TintMask.h`）有 `Type { ..., kSkinTone, ... }`（行 11-19）、`texture`/`color`/`alpha`（行 33-35）。
- **髮色/臉貼圖**：`headRelatedData`（行 269，`{ BGSColorForm* hairColor; BGSTextureSet* faceDetails; }`，行 122-123）；helper `SetHairColor(BGSColorForm*)`（行 261）、`SetFaceTexture(BGSTextureSet*)`（行 260）。
- **體格**：`height`（行 276）、`weight`（行 277）。
- **種族**：`TESRaceForm`（決定可用的 headparts/morph 範圍）。

**重新整理 3D / 套用外觀（拿到 actor ref 後）**：
- `Actor::DoReset3D(bool a_updateWeight)`（`A/Actor.h:520`）——本 repo `CustomizeNpc`（`NpcGenerator.cpp:109`）與 cookbook R5（`MODDING_COOKBOOK.md:136`）已用：改完 base 外觀要 reset 才生效。
- `Actor::Update3DModel()`（`A/Actor.h:633`）、`UpdateHairColor()`（行 634）、`UpdateSkinColor()`（行 636）——分項刷新。
- `Actor::Load3D(bool)`（`T/TESObjectREFR.h:302`，vfunc 6A）、`Set3D(NiAVObject*, bool)`（行 304，vfunc 6C）——重建 3D。
- `TESNPC::UpdateNeck(BSFaceGenNiNode*)`（行 264）——臉/頸接合。

**執行期可設 vs load 期烘焙**：
- ✅ **執行期可設並刷新**：tint、髮色、臉貼圖、weight/height、headParts 指標、faceData morph——改完 `DoReset3D(true)` / `Update3DModel()` 多半能套上（CustomizeNpc 已驗證 weight 這條）。
- ⚠️ **較吃 FaceGen 流水線**：完整臉型（FaceGen 把 morph+headparts+tint 烘成一張臉網格）在 vanilla 是 CK 預先烘 `.nif`/`.dds`（FaceGenData）。執行期硬改 morph 後的「即時重烘臉」是否每次都乾淨，是**要實測**的點（最壞情況：臉變 default 或暗臉/灰臉）。從**既有 NPC 模板 Copy** 可繼承一張已烘好的臉，最省事。

---

## 5. 「生成 base + 暫時 ref」hybrid（務實主推）

既然 base form 本身不可持久（§3），務實模式是：

> **co-save 存「食譜」，讀檔重建 base + 重擺 ref。**

這與 `PROCGEN_INTERIOR.md` §5.2 策略 B（存種子+食譜、讀檔重建）、`QUEST_ENGINE_DESIGN.md`「spawned 讀檔失效政策」（行 51、188）**完全同一條路**，也呼應 `MODDING_COOKBOOK.md:53`「動態造的東西不進存檔、`kNewGame`/`kPostLoadGame` 重給」。

**生命週期**：

1. **造**（session 內，§1）：`factory->Create()` → `Copy(template)` → 設外觀/名字/派系（§4）→ `PlaceObjectAtMe(base, false)`（temporary ref）。
2. **存**（`SetSaveCallback`）：把**重建食譜**寫進 co-save：模板 base 的**既有 plugin FormID**（`0xXXX~Skyrim.esm`，可被 `ResolveFormID` remap）、覆寫過的欄位（名字、性別、外觀參數、faction FormID 們）、擺放位置/cell、一個**你自己的穩定字串鍵**（如 `"court_wizard_victim"`）。**不要存** `0xFF......` 動態 FormID（重載即失效）也不要存 `TESNPC*`。
3. **讀**（`SetLoadCallback`）：讀回食譜 → 對所有 plugin FormID 跑 `ResolveFormID` → **重跑步驟 1 造一個新 base** → 重設外觀 → 重擺 ref。用你的字串鍵把「邏輯上的同一個 NPC」對回來。

**這樣會壞什麼（要設政策）**：
- **持久 ref handle 斷**：上個 session 的 actor ref（`ObjectRefHandle`）重載後失效。重擺的是**新的 ref**（新 `0xFF` ref FormID）。任何外部記著舊 ref FormID 的東西都要靠你的字串鍵重對。
- **quest alias / relationship 指向消失的 base**：原 base 沒了，指向它的 alias（`Actor::CheckForCurrentAliasPackage`，`NPC_BEHAVIOR_DEEP_MODIFICATION.md` 提及 vtable 0x049）、`TESNPC::relationships`（`T/TESNPC.h:295`）全斷——重建後要重新填 alias / 重設 relationship。
- **NPC 身上的狀態流失**：玩家對它做過的事（殺了沒、拿了什麼、好感度）若沒進你的食譜就沒了。要存的 delta 全得自己列。
- **時機**：重建必須在 `kPostLoadGame`（且 form 系統就緒、`kDataLoaded` 之後），擺放動遊戲狀態要包進 `SKSE::GetTaskInterface()->AddTask`（`MODDING_COOKBOOK.md` R9）。

**對「持久身分」的誠實話**：這個 hybrid 給的是「**邏輯上的持久身分**」（靠你的字串鍵 + 食譜重現），**不是引擎層的穩定 FormID 身分**。對 quest 用途（「同一個受詛咒侍從」）足夠；對「要被其他 mod 用 FormID 永久參照」則不行。

---

## 6. 替代方案

| 方案 | 怎麼做 | 可行性 / 持久性 | 適用 |
|---|---|---|---|
| **(A) co-save 食譜 + 重建（§5）** | 存食譜，讀檔重造 base + 重擺 ref | ✅ 可行；邏輯持久（非引擎 FormID 持久） | **本專案首選**，量身 NPC、可控外觀 |
| **(B) 複用/改寫既有 vanilla base** | 直接拿一個 vanilla `TESNPC`（如某 generic NPC）`PlaceObjectAtMe` 出 ref，再對 **ref/actor 層**做覆寫（AV、裝備、甚至 `actor->GetActorBase()` 微調） | ✅ 最穩；base 是 plugin form，**FormID 天生持久** | 不需要「獨一無二的 base」、可接受外觀撞臉時 |
| **(C) `TESLevCharacter`（leveled actor）** | `class TESLevCharacter : ... TESLeveledList`（`T/TESLevCharacter.h:10-18`，`FORMTYPE = FormType::LeveledNPC`）。引擎依等級從清單抽一個 base 實體化 | ⚠️ 仍是「執行期造 LevList = 動態 form」同樣不持久；用**既有** vanilla LevChar 才持久 | 想要「隨機/分級」的雜兵生成 |
| **(D) actor/AV 覆寫在既有 base 上** | 不造 base，只 `PlaceObjectAtMe` 既有 base 後改 actor 層（`ModActorValue`/`SetActorValue`、裝備、`DoReset3D` 改 weight），外觀差異靠 ref 層 | ✅ 穩；但**外觀自由度低**（共用 base 的臉/headparts） | 只需數值/裝備差異的群眾 NPC |
| **(E) 框架/工具（ESP/ESL 層）** | EasyNPC、Synthesis、AVO（Appearance）等 | 持久但**在 plugin 檔層運作，不是執行期 C++** | 不符本專案「不做 ESP」核心，僅作對照 |

**為什麼 EasyNPC / Synthesis / AVO 都在 plugin-file 層而非執行期？**（§7 查證）——因為**唯一能產生「跨 save/reload 穩定 FormID 身分」的 NPC base 的途徑，就是把它寫進一個 plugin（ESP/ESL/ESM）並排進 load order**。引擎只對「load order 裡的 form」保證穩定 FormID 與完整序列化。執行期 `0xFF` 動態 form 拿不到這個保證——這就是社群共識「沒有 ESP 就沒有持久 NPC」的技術根因。Synthesis（Mutagen）/ EasyNPC 是**離線**改/合成 plugin，不是執行期。

---

## 7. 先例（web；已驗證 vs 道聽途說）

- **「Form Factory 造的 form，依型別決定會不會被遊戲序列化」**——**已驗證**（Dynamic Form Tracker / DPF 作者文件、社群討論）。具名：**WEAP / ARMO / MISC 會被序列化**（玩家附魔物需要）；**food 等不會**。會被序列化者，「load game 之初除 form type 外無任何資料——空名、無模型、0 重量；script 仍掛著」。→ 直接佐證 §3.2：`FormType::NPC` 不在序列化清單，且就算在也只剩空殼。
- **Dynamic Form Tracker (DFT, Nexus 118995)**——**已驗證**：一個給 DLL 作者 include 的 header，**用 Form Factory 造 form 並建一個「form bank」**，可給自訂 ID 之後 fetch；明言能「**revive 不存在的 form**」、revive world 裡的 object ref 再 Disable+Enable 重現。→ **這就是 §5 hybrid 的社群既有實證**：DFT 的「bank + custom ID + revive」＝本報告的「食譜 + 字串鍵 + 重建」。它的存在本身證明「**動態造的 form 不會自己持久，需要框架在讀檔時重造**」。
- **DPF – Dynamic Persistent Forms (Nexus 116001)**——**已驗證存在**：「改變 form id 被序列化的方式」，需刪 `DynamicPersistentFormsCache.bin`、不可用舊存檔。→ 佐證「動態 form 持久化是個**需要外掛框架特別處理**的已知難題」，非引擎天生支援。
- **skaar wiki「Understanding Forms…」**——**已驗證**：Form 存進存檔、ObjectReference 不存；ref 靠 ReferenceAlias 或 `PlaceAtMe(...true)` 變持久。**但該頁未涵蓋「動態造的 base form 是否存活」**（明確查到：no coverage）。→ 不能拿它當「動態 base 可持久」的依據。
- **本 repo `NpcGenerator::SpawnNpc`**——**已驗證**：`factory->Create()<TESNPC>` + `Copy` + `PlaceObjectAtMe` 能編譯/連結並在 session 內運作（dynamic spell 觸發）。**未驗證**：save/reload 後該動態 base 是否還在（本報告判定：default NO）。
- **「執行期造的 NPC 能像 ESP NPC 一樣持久」**——**未見任何可信先例**；反向證據（DFT/DPF 的存在、序列化型別清單、社群共識）一致指向「**不行，需框架重建**」。列為**已驗證的否定**。

**Sources**
- [Dynamic Form Tracker (Nexus 118995)](https://www.nexusmods.com/skyrimspecialedition/mods/118995)
- [DPF – Dynamic Persistent Forms (Nexus 116001)](https://www.nexusmods.com/skyrimspecialedition/mods/116001)
- [Understanding Forms, Object References, Reference Aliases, and Persistence (skaar wiki)](https://github.com/xanderdunn/skaar/wiki/Understanding-Forms,-Object-References,-Reference-Aliases,-and-Persistence)
- [How Skyrim loads forms — David J Cobb](https://davidjcobb.github.io/articles/how-skyrim-loads-forms)（僅涵蓋 plugin 載入期，不涵蓋 save/dynamic form）
- [SKSE-Frameworks 清單 (GroundAura)](https://github.com/GroundAura/SKSE-Frameworks)
- [SKSE readme/whatsnew (silverlock)](https://skse.silverlock.org/skse_whatsnew.txt)

---

## 8. 直白裁決（blunt verdict）

**SKSE C++ 能做到**：
- ✅ 執行期**鑄造一個真正的 `TESNPC_` base form**（`IFormFactory` 或 `CreateDuplicateForm`），它拿到 `0xFF......` 動態 FormID。**本 repo `SpawnNpc` 已在做。**
- ✅ 從模板 `Copy` 出一個完整可用的 NPC，**程序化驅動其外觀**（race/headparts/faceData morph/tint/髮色/weight + `DoReset3D`/`Update3DModel` 刷新）。
- ✅ 用這個 base `PlaceObjectAtMe` 擺一個 actor，當下 session 內完全正常（會渲染、能戰鬥、能跑 §`NPC_BEHAVIOR…`的 AI package 注入）。

**SKSE C++ 不能可靠做到**：
- ❌ **讓這個全新 base form 跨 save/reload 自己存活、且保有穩定 FormID 身分（像 ESP 作者的 NPC）。**

**精確技術原因**：
1. 動態 FormID（`nextID` 配的 `0xFF......`，`T/TESDataHandler.h:175`）是**執行期計數器、session-local**，不保證跨重載穩定。
2. 存檔 codec **只序列化特定 form type**（WEAP/ARMO/MISC 級），**`FormType::NPC` 不在內**——重載後該 base 直接消失，指向它的 ref/alias/relationship 全懸空。
3. 即便某型別被序列化，重載後**只剩 form type、資料全空**（無名/無模型）。
4. `SKSE::SerializationInterface::ResolveFormID` **只能 remap 既有 plugin form 的號**，**無法復活你發明的全新 base**。
5. 「穩定 FormID 身分」這個保證**只屬於 load order 裡的 plugin form**——這就是為何所有 NPC 框架（EasyNPC/Synthesis/AVO）都在 **plugin 檔層**運作。

---

## 9. 務實建議模式（含 caveats）

**首選：co-save 食譜 + 讀檔重建 hybrid（§5）。**
- 沿用 `NpcGenerator::SpawnNpc` 的 `factory->Create()<TESNPC>` + `Copy(template)` 造法。
- co-save 只存**重建食譜**（模板的 plugin FormID + 覆寫欄位 + 外觀參數 + 位置 + **自訂字串鍵**），**絕不存** `0xFF` FormID 或裸指標。
- 讀檔（`kPostLoadGame` + `AddTask`）重跑造法，靠字串鍵維持「**邏輯上的同一 NPC**」。
- **Caveats**：給的是邏輯持久非引擎 FormID 持久；ref handle / alias / relationship / 玩家對它做的 delta 都要自己存進食譜並重建；外部用 FormID 永久參照它的需求滿足不了。

**次選（更穩、外觀讓步）：複用既有 vanilla base（§6 B/D）。**
- 直接擺既有 NPC base 的 ref，差異化做在 actor/ref 層。base 是 plugin form，FormID 天生持久，零持久化煩惱。撞臉是代價。

---

## 10. 最小 PoC + 最該先解的單一未知數

**最該先解的單一未知數（先做這個，其餘設計都依賴它）**：

> **一個 `factory->Create()<TESNPC>()`（或 `CreateDuplicateForm`）造出的 `TESNPC` base，
> 經一次 hard-save → 退到主選單 → reload 之後，到底還在不在？
> 它擺出的那個 actor ref 重載後是否消失/壞掉（無模型/暗臉/crash）？**

本報告依 §3 + §7 證據**預判答案是「不在 / 壞掉」**——但這正是 `ALCHEMY_SPIKE_FINDINGS.md` Test B 對 potion 留的同一個必測項，必須在遊戲裡親眼確認後才能把整個 NPC 持久化策略釘死。

**最小 PoC（照 alchemy spike 風格，按一鍵 + 全程 log）**：
1. 接一個輸入鍵（或新增一條 `"C++: Spawn Persistent NPC"` 法術，照 `NpcGenerator::CreateSpell` 模式）。
2. 觸發時：`factory->Create()<TESNPC>` → `Copy(genericTemplate)` → `fullName = "POC Persistent NPC"` → `PlaceObjectAtMe`，**log 出新 base 的 `0xFF` FormID 與 actor ref FormID**。
3. **Test 持久化**：hard-save、記下兩個 FormID → reload → 用 `LookupByID` 查那個 base FormID 還在不在、actor ref 是否還在場且有模型/有名字。
   - **若不在/壞掉**（預期）→ 確認必走 §5 hybrid，著手實作 co-save 食譜 + `kPostLoadGame` 重建，並驗證「重建後字串鍵能對回邏輯 NPC」。
   - **若意外還在且完整**（不預期）→ 再測「換 load order / 多次 reload 後 FormID 是否仍穩」才能下結論（很可能只是同 session 殘留假象）。
4. **Test 外觀**（與持久化解耦，可並行）：對擺出的 NPC 改 weight + 一個 tint layer + 換一個 headpart，`DoReset3D(true)` / `Update3DModel()`，肉眼確認外觀變了且臉沒壞（驗 §4 的「即時重烘臉」風險）。

**為什麼這個 PoC 好**：①全用 `NpcGenerator` 已驗證原語，零新風險原語；②單一變因直擊命門（base 活不活）；③與 alchemy spike 的測法同構，可複用其 log/觸發骨架；④外觀測試獨立，不被持久化結論卡住。

---

## 11. 接進本 repo JSON 引擎：`generate_npc` 作為 adapter 擴充動作

本 repo 的設計**已經把這條路鋪好了**——`COURT_WIZARD_DESIGN.md` 的「受詛咒侍從」就是 `spawn` 綁定、走 `NpcGenerator`（行 44、123、166、216），`QUEST_ENGINE_DESIGN.md` §2.1 定義了 `{ "bind": "spawn", "template": "0x...", "name": "..." }`（行 46、50）與 `spawn_character` 擴充動作（行 61）。本報告補的是「**這個 spawn 的持久化語意**」。

對齊 `QUEST_ENGINE_SPEC.md` §4.4 的「核心詞彙 + adapter 擴充」兩層模型（spec 行 143、190 已舉 `spawn_character`/`give_item` 為 Skyrim adapter 擴充例），程序化 NPC 自然落為一個**更豐富的 Skyrim-only 擴充動作 `generate_npc`**（`spawn_character` 的進階版：帶外觀/數值食譜）：

```jsonc
// 劇情或 characters 區塊裡的實體綁定（adapter 解讀，對核心不透明）
{
  "victim": {
    "bind": "spawn",
    "template": "0x00013BBF~Skyrim.esm",   // 既有 plugin FormID → 可被 ResolveFormID remap
    "name": "受詛咒的侍從",
    "recipe": {                            // generate_npc 的外觀/數值食譜（§4）
      "sex": "female",
      "race": "0x00013746~Skyrim.esm",     // 可選；省略則繼承模板
      "weight": 40.0,
      "tint_layers": [ { "index": 0, "color": "0xRRGGBB", "alpha": 0.6 } ],
      "hair_color": "0x...~Skyrim.esm",
      "factions": [ { "faction": "0x...~Skyrim.esm", "rank": 0 } ],
      "av": { "health": 80, "destruction": 25 }
    },
    "persist_key": "court_wizard_victim"   // §5 的穩定字串鍵；讀檔依此重建
  }
}
```

**adapter 端落地（Skyrim adapter ActionRunner，呼應 `QUEST_ENGINE_DESIGN.md` §2.1）**：
1. 收到 `spawn`/`generate_npc` → 對所有 plugin FormID（template/race/faction/hairColor）跑 `FormUtil::Parse` / 載入期 `ResolveFormID`。
2. `factory->Create()<TESNPC>` → `Copy(template)` → 套 `recipe`（性別 flag、race、weight、tint/hair、faction、AV，見 §4）。
3. `PlaceObjectAtMe(base, false)` → `DoReset3D(true)` 套外觀。
4. spawned handle + `recipe` + `persist_key` → 寫進 quest engine 的**進度 blob**（`QUEST_ENGINE_DESIGN.md` 行 114「adapter 把 blob 寫進 .skse co-save」；spec §6.2 行 230）。
5. 讀檔（`kPostLoadGame`）→ 從 blob 取 `recipe` → **重跑 2-3 造一個新 base + 新 ref** → 用 `persist_key` 對回邏輯實體。

**對齊點**：① entity ref / 綁定描述對核心不透明、adapter 解析（spec 行 43、210，design 行 41-51）；② 進度 blob 走 co-save（spec 行 230，design 行 114）；③ 擴充詞彙宣告 + 有效 schema 驗證（spec §4.4）——CLI harness 不宣告 `generate_npc`，引用它的劇情在 CLI 驗證期被擋（spec 設計意圖）；④ **直接落地 `QUEST_ENGINE_DESIGN.md` 行 51、188 的「spawned 讀檔失效政策」**——本報告 §5 的「讀檔重建 + 字串鍵重對」就是該政策的具體實作，建議把預設政策定為「**依 persist_key 重生**」（而非中斷）。

> **與 `COURT_WIZARD_DESIGN.md` 的銜接**：該 design 已把受詛咒侍從定為 `spawn`，但用了占位 `template`（`0x00013BBF`，行 192 標占位）。本報告建議：(a) 把占位換成一個合適的 vanilla 模板 NPC；(b) 明確採 §5 hybrid 並把 `persist_key` 加進綁定；(c) 召喚循環會 `reset_quest` 重排（design 行 171），每輪重生侍從正好天然契合「session-local base + 食譜重建」——**不需要侍從的 base 跨存檔持久，只需要每次召喚時依食譜重造**，這讓本題的硬限制在 court-wizard 用例裡**幾乎不痛**。

---

### 引用標頭/檔案（皆實際查閱）

repo：`src/NpcGenerator.cpp`（`SpawnNpc` 60-99：`factory->Create()<TESNPC>` 64 / `Copy` 75 / `PlaceObjectAtMe` 78；`SpawnAtLocation` 30-56；`PlaceTree` 154-191；`CustomizeNpc` 101-117：`DoReset3D` 109；`InitializeMagic` 241-310：`EffectSetting`/`SpellItem` factory 262-268、282）、`src/NpcGenerator.h`、`research/ALCHEMY_SPIKE_FINDINGS.md`（行 32、104-135 §2 持久化、Test B 192-207）、`research/NPC_BEHAVIOR_DEEP_MODIFICATION.md`、`research/PROCGEN_INTERIOR.md`（§1 CELL `CreateDuplicateForm` 封死、§5.2 策略 B、§5.3 co-save）、`CLAUDE.md`、`MODDING_COOKBOOK.md`（R4 NPC 複製 118-126、R5 外觀 128-137、R9 AddTask 176-182、行 53 動態 form 不進存檔）、`QUEST_ENGINE_SPEC.md`（§4.4 詞彙兩層 143、190；§6.2 blob/co-save 230；§5.1 entity ref 43）、`QUEST_ENGINE_DESIGN.md`（§2.1 spawn 綁定 41-51、`spawn_character` 61、blob co-save 114、spawned 失效政策 51/188）、`COURT_WIZARD_DESIGN.md`（受詛咒侍從 spawn 44/123/166/216、占位 FormID 192、reset_quest 循環）。

CommonLibSSE-NG headers：
- `T/TESNPC.h`（`class TESNPC`/繼承 31-36、`FORMTYPE=NPC` 42、`Copy` vfunc 217、`ChangeFlags` 44-62、`Skills` 75-111、`FaceData`/`Morphs`/`Parts` 134-188、`Layer`(tint) 190-203、`HeadRelatedData` 113-125、helper `ChangeHeadPart` 240/`GetHeadPartByType` 244/`SetHairColor` 261/`SetFaceTexture` 260/`SetSkinFromTint` 262/`UpdateNeck` 264、members `playerSkills` 267/`npcClass` 268/`headRelatedData` 269/`combatStyle` 271/`originalRace` 274/`height` 276/`weight` 277/`defaultOutfit` 281/`defaultPackList` 283/`crimeFaction` 284/`headParts` 285/`numHeadParts` 286/`bodyTintColor` 292/`relationships` 295/`faceData` 296/`tintLayers` 297、`sizeof 0x268` 302）。
- `T/TESActorBase.h`（繼承 12 元件 19-31、`SaveGame`/`LoadGame` vfunc 41-42、`AddChange`/`RemoveChange` 39-40、`sizeof 0x150` 64）。
- `T/TESActorBaseData.h`（`ACTOR_BASE_DATA::Flag`(kFemale/kEssential/…) 19-44、`TEMPLATE_USE_FLAG` 46-63、members `actorData` 125/`voiceType` 127/`baseTemplateForm` 128/`factions` 130、`sizeof 0x58` 132）。
- `T/TESForm.h`（`CreateDuplicateForm` vfunc 09 行 132、`AddChange`/`RemoveChange`/`CheckSaveGame`/`SaveGame`/`LoadGame`/`InitLoadGame`/`FinishLoadGame` 133-141、`RecordFlags`(kPersistent/kTemporary/kInitialized…) 50-106、`InGameFormFlag`(kForcedPersistent…) 108-117、`GetFormID` 288/`IsDynamicForm()` `>=0xFF000000` 326、`formID` member 354）。
- `I/IFormFactory.h`（`GetConcreteFormFactoryByType<T>` 40、`Create()` 42）+ `src/RE/I/IFormFactory.cpp`（`Create→CreateImpl` 23、`GetFormFactoryByType` 16）+ `C/ConcreteFormFactory.h`（`CreateImpl` override、`Create()` 內 static_cast）+ `RE/Offsets_VTABLE.h:926`（`VTABLE_ConcreteFormFactory_TESNPC_43_` 確認 TESNPC 工廠存在）。
- `T/TESDataHandler.h`（`AddFormToDataHandler` 43、`CreateReferenceAtLocation` 72、`formArrays[FormType::Max]` 167、`nextID` 175 offset 0xD50）。
- `B/BGSHeadPart.h`（`HeadPartType`(kMisc/kFace/kEyes/kHair) 33-38、`type` member 85）。
- `T/TintMask.h`（`Type`(…/kSkinTone) 11-19、`texture`/`color`/`alpha`/`type` 33-36）。
- `A/Actor.h`（`DoReset3D` 520、`Update3DModel` 633、`UpdateHairColor` 634、`UpdateSkinColor` 636、`GetActorBase` 524-525、`GetRace` 557）。
- `T/TESObjectREFR.h`（`Load3D` vfunc 6A 行 302、`Set3D` vfunc 6C 行 304、`CreateDuplicateForm` override 216、`PlaceObjectAtMe` 461）。
- `T/TESLevCharacter.h`（`class TESLevCharacter` 10、繼承 `TESLeveledList` 12、`FORMTYPE=LeveledNPC` 18）。
- `SKSE/Interfaces.h`（`SerializationInterface` 81、`Set{Save,Load,Revert,FormDelete}Callback` 96-99、`WriteRecord` 101、`OpenRecord` 125、`WriteRecordData` 127、`ReadRecordData` 153、`ResolveFormID` 177、`ResolveHandle` 178）。
- 對照（CELL 封死 `CreateDuplicateForm`）：`T/TESObjectCELL.h:181`（`{ return 0; }`）。
