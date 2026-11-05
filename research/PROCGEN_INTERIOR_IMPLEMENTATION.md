# 程序化室內：分層判定、最小範例、JSON adapter 與先例

[返回總索引](PROCGEN_INTERIOR.md)

## 7. 分層判定 + 最小範例設計

### 7.1 分層可行性判定（含工/險）

| 能力 | 可行性 | 工時 | 風險 | 依據 |
|---|---|---|---|---|
| 擺模組化殼件（STAT/MovableStatic）拼房間 | ✅ 可行 | 中 | 低（接縫需調 offset） | §2，`NpcGenerator` 已驗證同款原語 |
| 擺家具，玩家可坐/睡/用 | ✅ 可行 | 低 | 低 | §3.1，marker 隨模型 |
| **NPC 自己走去用家具/idle** | ⚠️ 部分 | 高 | **高（無 navmesh）** | §3.1，依賴 `NAVMESH_FREE_PATHFINDING.md` 自製移動 |
| 擺燈、補室內照明 | ✅ 可行 | 低 | 低-中（分區不正確） | §3.3,§6 |
| 雜物 keyframed/fixed 擺放 | ✅ 可行 | 低 | 低 | §4 |
| 雜物 havok 動態落定 | ⚠️ 可試 | 中 | 中（抖/穿，要 PoC） | §4 |
| 存檔重現（種子+食譜+co-save） | ✅ 可行 | 中 | 低-中 | §5，複用 quest engine co-save |
| 靠引擎存幾百 persistent ref | ⚠️ 不建議 | 低 | 中-高（脹檔/不穩） | §5.2 |
| **執行期建新 CELL** | ❌ 不可行 | — | — | §1，`T/TESObjectCELL.h` |
| **執行期建 portal/roombound/光照分區** | ❌ 不可行 | — | — | §6 |
| **執行期生成 navmesh** | ❌ 不可行 | — | — | `NAVMESH_FREE_PATHFINDING.md` |

### 7.2 最小範例：一個「施法生成小屋」的法術（後續實作 spec）

掛在 `NpcGenerator` 既有骨架上，照 `MODDING_COOKBOOK.md` R1 加：

```
法術名（fullName）："C++: Generate Room"   // SpellCastHandler 依此分派
型別：kLesserPower / kFireAndForget / kSelf（同 NpcGenerator::CreateSpell）
```

**演算法（在 `ProcessEvent` → `GenerateRoom(anchor)`，主執行緒，全程寫 log）**：

1. **定錨**：`O = anchor->GetPosition()`，`θ = anchor->data.angle.z`；`cell = anchor->GetParentCell()`，`world = anchor->GetWorldspace()`（同 `NpcGenerator.cpp:50`）。把生成點推到玩家面前 ~300 單位（forward vector，`NpcGenerator.cpp:36-39`）。
2. **擺地板（grid）**：`FarmFloor01`/`WoodFloor` 類 STAT，3×3 格、格距 = 量測值（如 256）。每格 `localOffset = {gx*step, gy*step, 0}` → `worldPos = O + Rz(θ)·localOffset`（`MathUtil::Angle::RotateVector`）→ `CreateReferenceAtLocation(floorBase, worldPos, {0,0,θ}, cell, world, ..., false, true)`。
3. **擺四面牆**：沿地板邊界放牆 STAT，朝向 = `θ + 該邊朝內角`；留一格當門口。
4. **擺天花板/屋頂**（模型 B 必要）：頂部 STAT 蓋住。
5. **擺家具（anchor-point）**：北牆放一張 `BedDouble`（`TESFurniture`，`kCanSleep`），中央放 `WoodChair01`×2 + `WoodTable`；各自相對 transform 由模板給。`SetMotionType(kFixed)` 釘死。
6. **擺燈**：天花板/牆上各一盞帶模型的 `TESObjectLIGH`（如 `Candlelight`/`FireLantern`），補光。
7. **擺雜物**：桌面 AABB 內用 seed 亂數散 2-4 件 MISC（盤/瓶/書），預設 `kKeyframed`。
8. **收尾**：把所有生成 ref 的 `ObjectRefHandle` 收進一個 `std::vector` 存起來（供清除/存檔）；寫一筆 co-save 記錄（模板 ID、`O`、`θ`、seed）。

**為什麼這個範例好**：① 全用 `NpcGenerator` 已驗證的原語，零新風險原語；② 不碰 CELL/portal/navmesh 三大硬限制；③ 直接示範食譜→世界座標的數學與 co-save 重建；④ 玩家能進去坐/睡（不依賴 NPC pathing）。

---

## 8. JSON 驅動角度：`generate_interior` 作為 quest engine 的 adapter 擴充動作

`QUEST_ENGINE_SPEC.md` §4 把詞彙分「核心 MUST」與「**adapter 擴充**」兩層（spec 行 143、187-190）：adapter 向核心宣告它支援的擴充動作 + 其參數 schema，核心對「有效 schema = 核心 ∪ adapter 擴充」做驗證（行 188）。spec 行 190 已舉 `spawn_character`/`give_item` 這類 Skyrim adapter 擴充動作為例。**程序化室內天然就是再加一個 Skyrim-only 擴充動作 `generate_interior`**：核心引擎不認得它（CLI harness 用到它的劇情會在驗證期被擋，正是 spec §4.4 想要的行為），只有 Skyrim adapter 把它接到本報告 §7 的 C++ 實作。

**房間模板（room template）JSON 形狀草案**（與 spec 的 entity-ref 不透明字串、adapter 解析慣例對齊；件用 `"editorid"` 或 `"formid~mod"` 字串，後者走 `FormUtil::Parse`）：

```jsonc
// 一份可重用的房間模板（與劇情 JSON 分離，由 generate_interior 動作引用）
{
  "template_id": "small_wood_house_v1",
  "version": 1,
  "anchor": { "mode": "in_front_of", "ref": "player", "distance": 300.0 },
  "grid_step": 256.0,                 // 此 kit 的量測格距（單位）
  "shell": [
    // base: editorid 或 "0x..~Mod.esp"；pos 為相對錨點的格座標或單位；rot 為「度」(adapter 轉弧度)
    { "base": "FarmHouseFloor01", "grid": [0,0], "z": 0,   "rot_deg": [0,0,0] },
    { "base": "FarmHouseFloor01", "grid": [1,0], "z": 0,   "rot_deg": [0,0,0] },
    { "base": "FarmHouseWall01",  "grid": [0,1], "z": 0,   "rot_deg": [0,0,0],   "motion": "fixed" },
    { "base": "FarmHouseRoof01",  "grid": [0,0], "z": 384, "rot_deg": [0,0,0] }
  ],
  "furniture_slots": [
    // 給相對 transform；adapter 擺成 ref 並 SetMotionType(fixed)
    { "slot": "bed",   "base": "BedDoubleVampireLeft", "pos": [0, 240, 0], "rot_deg": [0,0,180], "usable": true },
    { "slot": "chair", "base": "CommonChair01",        "pos": [64, 0, 0], "rot_deg": [0,0,90],  "usable": true }
  ],
  "lights": [
    { "base": "DefaultCandleLight01NS", "pos": [0, 0, 300], "rot_deg": [0,0,0] }
  ],
  "clutter": [
    // count + seed 散佈在指定 AABB 內；motion 預設 keyframed，可選 dynamic 落定
    { "base": "Tankard01", "scatter_aabb": [[-48,180,200],[48,300,200]], "count": 3, "seed": 1337, "motion": "keyframed" }
  ],
  "navmesh_note": "新區域無 runtime navmesh；NPC 用家具須配合 NAVMESH_FREE_PATHFINDING 自製移動"
}
```

**劇情 JSON 裡引用它（adapter 擴充動作）**：

```jsonc
{
  "action": "generate_interior",        // Skyrim adapter 擴充，CLI adapter 不宣告 → 用到它的劇情在 CLI 驗證期被擋
  "template": "small_wood_house_v1",
  "at": { "ref": "player", "mode": "in_front_of", "distance": 300.0 },
  "persist_key": "court_wizard_lab"     // 寫進 co-save 的鍵；讀檔依此 + seed 重建（§5.3）
}
```

**adapter 端落地**：Skyrim adapter 的 ActionRunner 收到 `generate_interior` → 載入對應 template JSON →（度轉弧度 `DegreeToRadian`、相對轉世界 `RotateVector`）→ 逐件 `CreateReferenceAtLocation`/`PlaceObjectAtMe` → `SetMotionType` → 收 handle → 用 `persist_key` 寫 co-save（`SerializationInterface`，§5.3）。**驗證模型**：adapter 向核心宣告 `generate_interior` 的參數 schema（spec 行 187），核心對「有效 schema」驗證；不支援它的 adapter（如 CLI harness）載到引用它的劇情會在驗證期擋下（spec 行 320 的設計意圖）。

> 對齊點：① entity ref（`"player"`、件字串）對核心不透明、adapter 解析（spec 行 43、§5.1）；② 進度 blob 走 co-save（spec 行 230）；③ 擴充詞彙宣告 + 有效 schema 驗證（spec §4.4、4.3）。程序化室內**不需要動核心 schema**，純粹當一個 Skyrim 擴充動作插進去。

---

## 9. 先例（web；已驗證 vs 道聽途說）

- **Papyrus `PlaceAtMe(form, count, forcePersist, initiallyDisabled)`**——已驗證（Papyrus 原生 + SKSE）：本報告 C++ 的 `PlaceObjectAtMe` 即其底層；`forcePersist=true` 回持久 ref。SKSE 1.7.3 加了 `SpawnerTask` 做**批次 PlaceAtMe + 定位**，正是「一次擺很多件」的引擎面背書（silverlock SKSE whatsnew）。佐證 §2 的擺放原語 + §5.1 的持久化參數語意。
- **PapyrusUtil SE / powerofthree's Papyrus Extender**——已驗證存在的成熟 SKSE plugin，提供大量 ref 操作/事件函式；說明「執行期大量操作 ref」是社群常規。
- **Placeable Statics – Move Anything / Jaxonz Positioner / Dylbill's Positioner / Object Manipulation Overhaul**——已驗證：玩家可執行期抓取、移動、旋轉、縮放、擺放幾乎任何 static/furniture/clutter。**直接證明「執行期擺放既有 base object 拼裝場景」完全可行**，正是本報告 §2-§3 的核心。
- **Object Placement Saver（用 JContainers SE 存 JSON）/ JContainers SE**——已驗證：把「擺了哪些件、各自 transform」序列化成 JSON 存檔重建。**這就是 §5.2 策略 B（存食譜、讀檔重建）+ §8（JSON 食譜）的 Papyrus 界既有實證**；本報告把它換成 SKSE co-save + C++，思路一致。
- **「Place Everywhere」具體該 mod 未在本次搜尋結果命中**——列為**道聽途說/待查**，不作結論；但同類定位工具（上條）已足以佐證可行性。
- **navmesh 限制**——已驗證共識（見 `NAVMESH_FREE_PATHFINDING.md` §6 引用 CK wiki）：生成區域無 navmesh，NPC 引擎 pathing 失效。本報告 §3.1、§7.1 據此把「NPC 自己走去用家具」標為高風險。

**Sources**
- [gamesas: Spawning objects like placeatme with Papyrus](https://www.gamesas.com/spawning-objects-like-placeatme-with-papyrus-t258098.html)
- [SKSE whatsnew (SpawnerTask, batch PlaceAtMe)](https://skse.silverlock.org/skse_whatsnew.txt)
- [Understanding Forms, Object References, and Persistence (skaar wiki)](https://github.com/xanderdunn/skaar/wiki/Understanding-Forms,-Object-References,-Reference-Aliases,-and-Persistence)
- [PapyrusUtil SE](https://www.nexusmods.com/skyrimspecialedition/mods/13048)
- [powerofthree's Papyrus Extender](https://www.nexusmods.com/skyrimspecialedition/mods/22854)
- [Placeable Statics - Move Anything](https://www.nexusmods.com/skyrimspecialedition/mods/342)
- [Object Placement Saver (JContainers JSON 存檔)](https://www.nexusmods.com/skyrimspecialedition/mods/141360)
- [JContainers SE](https://www.nexusmods.com/skyrimspecialedition/mods/16495)
- [Dylbill's Positioner SSE](https://www.nexusmods.com/skyrimspecialedition/mods/33917)
- [Object Manipulation Overhaul](https://www.nexusmods.com/skyrimspecialedition/mods/123664)

---

### 引用標頭/檔案（皆實際查閱）

repo：`src/NpcGenerator.cpp`（`SpawnAtLocation`:30-56、`PlaceTree`:154-191、`InitializeMagic`:241-310、`SpellCastHandler`:195-239）、`src/util.h`（`MathUtil::Angle` 227-293、`Interp::InterpTo` 332、`ObjectUtil::Transform` 352-388、`NifUtil::Node::Clone` 491、`NifUtil::Collision` 553-605、`FormUtil::Parse` 408-486）、`CLAUDE.md`、`MODDING_COOKBOOK.md`（§1.1、1.2、1.4、R1、R3、R6）、`QUEST_ENGINE_SPEC.md`（§4.2-4.4 行 143/187-190、§5.1 行 43/210、§6.2 行 230、附錄 A 行 319-320）、`research/NAVMESH_FREE_PATHFINDING.md`。

CommonLibSSE-NG headers：`T/TESObjectCELL.h`（113-273；CELL form、`CreateDuplicateForm`:181、`GetbhkWorld`:197、`GetLighting`:202、`SetFog*`:214-216、portalGraph:83、RUNTIME_DATA:224-239）、`T/TESObjectSTAT.h`（27-74）、`B/BGSMovableStatic.h`（24-66）、`T/TESFurniture.h`（10-137；ActiveMarker:17-51、Activate vfunc:125、WorkBenchData:68-87）、`B/BSFurnitureMarkerNode.h`（9-41）、`T/TESObjectLIGH.h`（56-112；OBJ_LIGH/flags:18-54,104）、`I/InteriorData.h`（8-45）、`T/TESDataHandler.h`（`CreateReferenceAtLocation`:72）、`T/TESObjectREFR.h`（MotionType:126-135、SetDelete:234、InitHavok:298、Set3D:304、MoveHavok:333、Disable:337、SetParentCell:352、CreateRefHandle:371、Enable:374、GetParentCell:413、GetPosition:414、GetWorldspace:425、HasCollision:426、IsDisabled:442、IsMarkedForDeletion:451、IsPersistent:453、MoveTo:456、PlaceObjectAtMe:461、SetActivationBlocked:464、SetCollision:465、SetMotionType:468、SetPosition:469-470）、`B/bhkRigidBody.h`（13-60；SetPositionAndRotation vfunc:45、SetLinearVelocity 等:52-55）、`B/BSPortalGraph.h`（12-37）、`B/BSMultiBoundNode.h`（GetMultiBoundRoom:50）、`T/TESWorldSpace.h`（portalGraph:200）、`B/BSLight.h`（portalGraph:71）、`A/ActorState.h`（GetSitSleepState:159）、`I/IFormFactory.h`（GetConcreteFormFactoryByType:40）、`SKSE/Interfaces.h`（SerializationInterface:81、SetSave/LoadCallback:97-99、OpenRecord:125、WriteRecord:101、ReadRecordData:153）。
