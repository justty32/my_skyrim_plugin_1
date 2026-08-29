# 程序化外景：最小實作、JSON adapter 與查證來源

[返回總索引](PROCGEN_EXTERIOR.md)

## 8. 最小範例設計：一個生成小城堡的 SPELL

> 目標：給後續實作用的規格。風格沿用 repo 既有的「自製 SpellItem + `TESSpellCastEvent` 攔截」模式
> （`src/NpcGenerator.cpp:195-310`），不需 CK/ESP。

**Spell**：`"C++: Conjure Keep"`（lesser power，self-cast；建立方式照 `InitializeMagic`
`src/NpcGenerator.cpp:281-304`）。

**用到的 kit 件**（用 EditorID 查，照 `LookupByEditorID`；以下為示意，實作前要在遊戲內查實際可用 EditorID）：
- 地基/平台 STAT ×N（填高整平）
- 直牆段 STAT（`wallStraight`）
- 轉角塔 STAT（`towerCorner`）
- 門樓 STAT（`gateHouse`）
- （選配）中央主樓 STAT（`keepKeep`）

**擺放／錨定演算法**（在 spell cast handler 內，主執行緒；用 `TaskInterface::AddTask` 分幀）：
1. `anchor = a_event->object.get()`（施法者）；`origin = anchor->GetPosition()`，
   往前 600 units 取結構中心（forward 用 `src/NpcGenerator.cpp:36-39` 的 sin/cos，或 `MathUtil` 版）。
   `baseAngleZ = anchor->data.angle.z`，`ws = anchor->GetWorldspace()`，`cell = anchor->GetParentCell()`。
   **若 `ws == nullptr`（室內）→ 中止 + log**（外景專用）。
2. **整平基準**：在預定 footprint（如 3×3 grid，每格 1024 units，總 ≈3072×3072）取若干取樣點，
   各做 `TES::GetLandHeight` 取 z，求 `zMax`；平台高度 = `zMax`。
3. **拼裝**（每件相對結構本地座標 → §2 旋轉平移到世界座標 → `CreateReferenceAtLocation(base, worldPos,
   {0,0,worldAngleZ}, cell, ws, nullptr, nullptr, ObjectRefHandle(), /*forcePersist=*/false, true)`）：
   - 先鋪地基件填到 `zMax`（外圈逐件 `GetLandHeight` 往下貼地填縫）。
   - 四邊各鋪直牆段（依 footprint 邊長 / 牆段長度 算段數，量化 snap）。
   - 四角擺轉角塔。
   - 前邊中段用門樓取代一段牆。
   - （選配）中心擺主樓。
4. 每幀擺 K 件（如 8 件/幀）直到清單空，避免一次擺幾十件卡頓/CTD。
5. log 每件 FormID（Debug build 依 MEMORY 偏好 aggressive log）。

**錨定/整平要點**：主體件統一用 `zMax`（齊平），只有地基外圈逐件貼地；牆段 z = 平台高度 + 件本地 z 偏移。
件**不 persist**；若要可重載，照 §5 存 `{recipeId, seed, ws FormID, origin, baseAngleZ}` 一筆 co-save record，
讀檔後在 cell attach 時重跑步驟 2–4。

**已知限制（要在 spell 說明/log 寫清楚）**：走遠城堡會消失（無 LOD）；NPC 不會自己走進去（無 navmesh）；
地面只是視覺整平（地形沒真的改）。

---

## 9. JSON 驅動角度：`generate_structure` adapter 擴充動作

> 接上 `QUEST_ENGINE_SPEC.md` §4.4 的 adapter 擴充機制與 §5.2 的 ActionRunner。外景 procgen 作為
> **adapter 宣告的擴充動作 `generate_structure`**，核心不理解其語意，只把（動詞、參數、已解析實體）轉交
> ActionRunner（即本 plugin 的擺放器）。形狀刻意與室內報告的 `generate_interior` 對齊（同樣 pieces +
> 相對 transform + anchor 規則），差別在外景多了 worldspace 錨點與整平規則。

**動作 JSON（劇情側，引用一個結構模板 id + 錨點 + seed）**：

```json
{
  "generate_structure": {
    "template": "small_keep",
    "anchor": { "ref": "player", "forward": 600, "use_facing": true },
    "seed": 1337,
    "ground": "flatten_to_max",
    "persist": "recipe"
  }
}
```

- `template`：指向下方「結構模板庫」的 id（adapter/內容自帶的資料）。
- `anchor`：錨點實體（走 EntityResolver §5.1 解析 `"player"`）、前方距離、是否沿用其朝向當 `baseAngleZ`。
- `seed`：決定論種子（任何隨機走 seeded RNG，§5）。
- `ground`：整平策略 — `flatten_to_max`（§2 基準面+地基）/ `per_piece`（逐件貼地）。
- `persist`：`recipe`（存 seed/recipe 讀檔重拼，§5）/ `none`（不持久）。

**結構模板（資料側，與 `generate_interior` 同骨架：pieces + 相對 transform + anchor rules）**：

```json
{
  "structures": {
    "small_keep": {
      "footprint": { "cells": [3, 3], "cell_size": 1024 },
      "ground_rule": { "mode": "flatten_to_max", "foundation_piece": "stoneFoundation01" },
      "pieces": [
        { "piece": "wallStraight01",  "at": [-1536, 1536, 0], "rot_z": 0,   "repeat": { "axis": "x", "step": 1024, "count": 4 } },
        { "piece": "towerCorner01",   "at": [-1536, 1536, 0], "rot_z": 0 },
        { "piece": "towerCorner01",   "at": [ 1536, 1536, 0], "rot_z": 90 },
        { "piece": "towerCorner01",   "at": [ 1536,-1536, 0], "rot_z": 180 },
        { "piece": "towerCorner01",   "at": [-1536,-1536, 0], "rot_z": 270 },
        { "piece": "gateHouse01",     "at": [    0, 1536, 0], "rot_z": 0,   "anchor_ground": true },
        { "piece": "keepKeep01",      "at": [    0,    0, 0], "rot_z": 0 }
      ],
      "piece_resolve": "editor_id"
    }
  }
}
```

- `at` = 結構本地座標（相對結構原點）；`rot_z` = 本地 Z 旋轉（度）。adapter 用 §2 數學轉成世界 transform。
- `repeat` = 沿軸重複（自動算牆段數，省手寫每段）。
- `anchor_ground` = 該件是否逐件貼地（外圈地基/門樓），否則用基準平台高度。
- `piece_resolve: "editor_id"` = 件用 EditorID 解析（複用 `LookupByEditorID`，免硬編 FormID）。
- `foundation_piece` = `flatten_to_max` 模式填縫用的地基 STAT。

**ActionRunner（本 plugin adapter 側）職責**：解析 `template` → 對每個 piece 跑 §2 transform → 依
`ground_rule` 算 z → `CreateReferenceAtLocation`（分幀）→ 依 `persist` 決定是否寫 co-save recipe（§5）。
adapter MUST 向核心宣告 `generate_structure` 的參數 schema（§4.4），驗證期才認得；不支援此擴充的 adapter
（如室內-only 或 headless CLI）會在驗證期擋掉用到它的內容 — 與規格的可攜性邊界一致。

> **與 `generate_interior` 的關係**：兩者共用「pieces + 相對 transform + 解析方式」骨架，可共用同一個
> 擺放器核心；外景版多 `footprint`/`ground_rule`/worldspace 錨點，室內版改成 cell-local 錨點且無整平/LOD/
> navmesh 包袱（室內 navmesh 一樣是烘焙、一樣不可走，見室內報告）。

---

## 10. 社群先例（已驗證 vs 道聽途說）

- **DynDOLOD（已驗證機制，道聽途說其細節）**：distant LOD 工具，用「persistent + Is Full LOD flag 的
  neverfade ref」+ NearGrid/FarGrid enable/disable 控制遠處顯示；DynDOLOD DLL（SKSE）用 native code 控制
  並修 LOD bug（fast travel 後 LOD 不卸載等）。**佐證**：①LOD/large-ref 是預烘焙+特殊 flag 的領域，純執行期
  動態擺的 STAT 拿不到；②要更遠顯示得靠 enable/disable grid，代價是效能指數上升。
  （未逐項驗證其 API；屬機制佐證而非可直接抄的程式。）
- **「PlaceAtMe 大量 persistent → 存檔膨脹／FormID 耗盡」（已驗證社群共識）**：單次/數百次無害，**腳本反覆
  PlaceAtMe 會耗盡 2^24 FormID 池並 bloat/CTD**；persistent ref 不自動清理。佐證 §4/§5 的「別 persist 幾百件、
  存 recipe 重拼」。
- **Build Your Own Home / 各 player-home 建造 mod（道聽途說）**：證明「執行期擺現成 static 拼建築」社群長期在做，
  多為 Papyrus PlaceAtMe + MoveTo 對齊；但規模、對齊精度、效能受 Papyrus 限制 — **SKSE C++ 在「分幀擺幾百件、
  精準 transform、決定論重拼」上明顯優於 Papyrus**（Papyrus 慢、無 frame hook、難做大量同步擺放）。
- **Object Manipulation Overhaul（道聽途說）**：物件操作/擺放類 mod，佐證執行期 ref transform 操作是成熟領域。
- **「actor 需要 navmesh 才能走」（已驗證，見 NAVMESH 報告 §6 的 CK wiki/gamesas 來源）**：佐證 §6 城堡不可
  直接讓 NPC pathing。

> **Papyrus vs SKSE at scale**：擺幾百件、要分幀/主執行緒控制/co-save 二進位 recipe/監 cell attach 事件，
> 這些 Papyrus 要嘛做不到要嘛極慢；本 repo 的 C++/CommonLibSSE-NG 路線是正解。

---

## 引用標頭（皆實際開檔查證）

- `T/TESWorldSpace.h`（`cellMap:187`、`persistentCell:188`、`terrainManager:189`、`CellID:55`、
  `defaultLandHeight:220`、`Flag::kNoLandscape:131`）
- `T/TESObjectCELL.h`（`EXTERIOR_DATA:45`、`GetbhkWorld:197`、`ForEachReferenceInRange:199`、
  `RUNTIME_DATA cellLand/navMeshes/references/worldSpace/loadedData:226-237`、`CellState::kAttached:139`、
  `IsAttached/IsExteriorCell:209-210`）
- `T/TESObjectLAND.h`（`LoadedLandData::heights[4][289]:67`、`moppCode:76`、`geom[4]:71`、`OBJ_LAND::Flag:18`）
- `B/BGSTerrainManager.h`（全 `unkXX`，僅 `lodTreesHidden:16` 具名）
- `T/TESCellAttachDetachEvent.h`（`:10-16`）、`I/ICellAttachDetachEventSource.h`（`TES` 繼承之，`T/TES.h:37`）
- `T/TES.h`（`gridCells:82`、`worldSpace:126`、`GetCell:72`、`GetLandHeight:74`、`Pick:77`）
- `G/GridCellArray.h`（`GetCell:25`、`cells:33`）、`G/GridArray.h`（`length:25` ← uGridsToLoad）
- `T/TESObjectSTAT.h`（`FORMTYPE Static:34`、`RecordFlags::kHasDistantLOD/kHasTreeLOD/kAddOnLODObject:46-52`、
  `kNavMeshGeneration_*:50-55`、`TESObjectSTATData:9`）
- `T/TESDataHandler.h`（`CreateReferenceAtLocation:72`）
- `T/TESObjectREFR.h`（`PlaceObjectAtMe:461`、`SetPosition:469-470`、`SetAngle`、`GetScale:418`、
  `GetWorldspace:425`、`GetParentCell:413`、`SetParentCell:352`、`RecordFlags::kPersistent:181`、`refScale:476`）
- `SKSE/Impl/Stubs.h`（serialization `SetSaveCallback`/`OpenRecord`/`WriteRecordData`/`ReadRecordData`:72-79）
- repo：`src/NpcGenerator.cpp`（`CreateReferenceAtLocation` 用法:48-52、`PlaceObjectAtMe`:78,167、
  forward 向量:36-39、`LookupByEditorID`:128,159、spell 建立:281-304）、
  `src/util.h`（`MathUtil::Angle::RotateVector:283`、`GetForwardVector:291`、`NormalAbsoluteAngle:247`、
  `Clamp:202`、`Interp::InterpTo:332`、`Transform::TranslateTo:356`、`InterpAngleTo:363`、
  `NifUtil::Collision::ToggleMeshCollision:555`）
- repo docs：`research/NAVMESH_FREE_PATHFINDING.md`（§1 navmesh 烘焙、§2 char controller velocity、§4 貼地/射線）、
  `research/PROCGEN_INTERIOR.md`（持久化/動態 base form/`generate_interior` 對齊 — 姊妹報告）、
  `QUEST_ENGINE_SPEC.md`（§4.4 adapter 擴充、§5.1 EntityResolver、§5.2 ActionRunner、§8 決定論）、
  `COMMONLIBSSE_INDEX.md:33`（SerializationInterface）

**Sources（web，已驗證 vs 道聽途說已標註）**
- [DynDOLOD — Dynamic LOD 機制](https://dyndolod.info/Help/Dynamic-LOD)
- [DynDOLOD DLL（SKSE native）](https://dyndolod.info/Help/DynDOLOD-DLL)
- [DynDOLOD Large Reference Bugs Workarounds](https://dyndolod.info/Help/Large-Reference-Bugs-Workarounds)
- [PlaceAtMe — CreationKit Wiki](https://ck.uesp.net/w/index.php?title=PlaceAtMe_-_ObjectReference)
- [gamesas: PlaceAtMe & savegame bloat](https://www.gamesas.com/placeatme-savegame-bloat-question-t195132.html)
- [gamesas: PlaceAtMe vs AddItem saved game bloating](https://www.gamesas.com/placeatme-additem-saved-game-bloating-t256663.html)
- [Build Your Own Home (Nexus)](https://www.nexusmods.com/skyrim/mods/18480)
- [Object Manipulation Overhaul (Nexus)](https://www.nexusmods.com/skyrimspecialedition/mods/123664)
