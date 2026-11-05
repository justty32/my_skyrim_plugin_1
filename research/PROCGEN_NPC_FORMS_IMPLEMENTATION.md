# 程序化 NPC Form：最小 PoC、JSON adapter 與引用來源

[返回總索引](PROCGEN_NPC_FORMS.md)

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
