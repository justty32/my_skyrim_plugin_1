# 程序化 NPC Form：既有地基、鑄造與持久化難題

[返回總索引](PROCGEN_NPC_FORMS.md)

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
