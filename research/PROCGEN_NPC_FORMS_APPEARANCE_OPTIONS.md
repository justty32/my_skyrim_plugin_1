# 程序化 NPC Form：外觀、hybrid、替代方案與裁決

[返回總索引](PROCGEN_NPC_FORMS.md)

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
