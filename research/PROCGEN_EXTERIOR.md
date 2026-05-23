# 執行期程序化生成外部結構（城堡）與外景 — 可行性／設計分析

> 探索性可行性分析，2026-05-23。由 research agent 產出。所有引擎面論斷均對照本 repo vendored 的
> `CommonLibSSE-NG/include/RE/` 標頭查證並引實際檔案/行號。**僅分析，未動 plugin 程式碼**（不碰
> `src/`、`cmake/`、`plugin.cpp` — 由其他 agent 在獨立 worktree 同時編輯）。
>
> 範圍：**外景／worldspace 與大型結構**。室內（cell 內裝、`generate_interior`）由姊妹報告
> `research/PROCGEN_INTERIOR.md` 負責；本報告**交叉引用，不重複**室內共通的部分（持久化機制、
> 動態 base form 建立等），只補外景特有的差異。尋路／地面貼合／havok 射線等已在
> `research/NAVMESH_FREE_PATHFINDING.md` 推導過，本報告**引用其結論，不重新推導**。

## 結論先講（TL;DR）

- **可行（有重大限制）**：用 repo 既有的 `TESDataHandler::CreateReferenceAtLocation`／`PlaceObjectAtMe`
  （`src/NpcGenerator.cpp:48-52, 78`）把現成的模組化 `TESObjectSTAT`（牆/塔/門組件）逐件擺進
  **玩家當前所在的外部 worldspace 已載入 cell**，用 `MathUtil` 做 snap/旋轉、用
  `RE::TES::GetLandHeight` + havok 向下射線貼地，可在執行期拼出一座可見、可碰撞的城堡。
- **三個誠實的硬限制（無法繞過，只能緩解）**：
  1. **無遠景 LOD**：動態擺放的 ref **沒有 distant LOD**（LOD 是 CK/xLODGen 預先烘焙的，見
     `TESObjectSTAT::RecordFlags::kHasDistantLOD = 1 << 15`，`T/TESObjectSTAT.h:51`）。城堡只在
     active grid（`uGridsToLoad`，預設 5×5）內才看得見，超出即整塊消失，遠看沒有剪影、近看有 pop-in。
  2. **無執行期 navmesh**：拼出來的城堡 NPC 無法用引擎 pathing 走（navmesh 是烘焙 form，
     `RE::NavMesh : TESForm`，見 NAVMESH 報告 §1）。要 NPC 在城堡裡走，只能套 NAVMESH 報告的
     「自驅 locomotion」方案。
  3. **無法改地形 heightmap**：landscape 高度是烘焙在 `TESObjectLAND` 的 LAND 資料（見 §3），
     執行期實務上**不能改地形**；不平地面只能靠擺放地基/平台 static 來「視覺整平」。
- **持久化**：不要持久化幾百個 dynamic ref（FormID 池有限、存檔膨脹）。**存一顆 seed/recipe，讀檔重拼**
  ——與室內報告同策略但規模壓力更大（見 §5，交叉引用 `PROCGEN_INTERIOR.md` 的 persistence 段）。

---

## 1. 執行期的 worldspace vs cell — 能擺到哪裡

### worldspace / cell 的執行期結構（皆已 vendored）

- **`TESWorldSpace`**（`T/TESWorldSpace.h:114-236`）：是 `TESForm`。關鍵成員
  `BSTHashMap<CellID, TESObjectCELL*> cellMap`（`:187`）把 (x,y) cell 座標映到 cell；
  `persistentCell`（`:188`，整個 worldspace 的持久 cell）；`BGSTerrainManager* terrainManager`（`:189`）；
  `defaultLandHeight`（`:220`）。`CellID{y, x}`（`:55-87`）就是外部 cell 的整數格座標。
- **`TESObjectCELL`**（`T/TESObjectCELL.h:113`）：`EXTERIOR_DATA`（`:45-69`，XCLC）內含 `cellX/cellY`
  與 `worldX/worldY`（cell 左下角的世界座標）。`RUNTIME_DATA`（`:222-242`）內含
  `TESObjectLAND* cellLand`（`:226`）、`NavMeshArray* navMeshes`（`:228`）、
  `BSTSet<NiPointer<TESObjectREFR>> references`（`:229`，cell 內所有 ref）、`TESWorldSpace* worldSpace`（`:236`）、
  `LOADED_CELL_DATA* loadedData`（`:237`，只有載入時非空）。
- **cell 狀態旗標**：`cellState`（`:259`，`CellState::kAttached = 7`，`:139`）、`cellDetached`（`:261`）；
  方法 `IsAttached()`、`IsExteriorCell()`（`:209-210`）。**只有 attached 的 cell 其
  `loadedData` 才有效、ref 的 3D 才在場景圖裡**。

### 玩家周遭的已載入 cell 怎麼拿

- `RE::TES::GetSingleton()`（`T/TES.h:67`）：
  - `gridCells`（`:82`，`GridCellArray*`）就是「玩家周遭的 N×N 外部 cell 網格」。
    `GridCellArray::GetCell(x, y)`（`G/GridCellArray.h:25-30`）取格內 cell；`length`（`G/GridArray.h:25`，
    註解明寫「takes value from uGridsToLoad」）= 一邊的格數（預設 5）。中心格是玩家所在。
  - `worldSpace`（`T/TES.h:126`）= 玩家當前外部 worldspace。
  - `GetCell(const NiPoint3& pos)`（`:72`）：用世界座標反查 cell。
  - `GetLandHeight(posIn, heightOut)`（`:74`）：地形高度（貼地用，見 §2）。
  - `Pick(bhkPickData&)`（`:77`）：havok 射線（貼地備援，見 §2 與 NAVMESH 報告 §4）。

### 能不能擺進「尚未載入」的 cell？

- `CreateReferenceAtLocation`（`T/TESDataHandler.h:72`）的簽名收 `TESObjectCELL* a_targetCell` 與
  `TESWorldSpace* a_selfWorldSpace`。對外部 worldspace，**正確做法是傳 worldspace 並讓引擎依座標
  決定 cell**（room handle 留 `ObjectRefHandle()`）；repo 既有用法（`src/NpcGenerator.cpp:48-52`）
  正是這個模式（在室外傳 `GetWorldspace()`）。
- **擺進未載入 cell 的問題**：你可以查到一個未 attached cell 的 `TESObjectCELL*`（`cellMap` 一直在），
  但它的 `loadedData == nullptr`、`bhkWorld` 不存在（`GetbhkWorld()`，`:197`，回傳依賴 loaded data）。
  新建 ref 雖然能掛到該 cell 的 `references`/persistent list，但**它的 3D／碰撞要等 cell attach
  才會建立**。也就是說：
  - **務實邊界**：只在「玩家當前所在 + 周遭 active grid」內的 attached cell 擺東西，確保 3D/碰撞即時可見。
  - **大型結構跨多 cell 的麻煩**：一座城堡的 footprint 可能橫跨好幾個 cell。離玩家近的 cell 是 attached、
    遠的可能還沒 attach（grid 邊緣）。擺在尚未 attach 的 cell 的件，玩家走近才「冒出來」（attach 時建 3D）。
- **cell attach/detach 事件**：`TESCellAttachDetachEvent`（`T/TESCellAttachDetachEvent.h:10-16`）
  ——`{ NiPointer<TESObjectREFR> reference; bool attached; }`。注意它的 payload 是**單一 ref**（哪個
  ref 進/出 cell），不是「整個 cell attach」的訊號。要監看 cell 層級的 attach，事件源在
  `RE::TES : ICellAttachDetachEventSource`（`T/TES.h:37`、`I/ICellAttachDetachEventSource.h`）。
  - **用途**：把「重拼結構」掛在玩家移動進入目標 cell 的 attach 事件上 — 在 cell 變 attached 的當下
    才把該 cell 的件擺出來，避免擺進空 cell。也是讀檔後重拼的觸發時機（§5）。

> **load-distance / attach 問題（大型結構核心痛點）**：active grid 預設只有 5×5 cell（≈ 5×4096 units
> ≈ 一邊 20480 units）。一座大城堡若超出，外圈件根本沒被擺（cell 未 attach）或被 detach 後消失。
> **緩解**：① 把結構規模控制在玩家周遭 3×3 cell 內（最穩）；② 監看 attach 事件做「按 cell 分批擺放」；
> ③ 若硬要超大型，得自己管「已擺/待擺」清單，隨玩家移動補擺/回收 — 複雜度陡升，不建議初版做。

---

## 2. 從模組化 static 組件拼城堡

### 用什麼擺、怎麼擺

- **基礎生成原語**（repo 已驗證可靠，`src/NpcGenerator.cpp`）：
  - `TESObjectREFR::PlaceObjectAtMe(base, forcePersist)`（`T/TESObjectREFR.h:461`）— 在 anchor 位置生一個，
    再 `SetPosition`/`SetAngle` 搬到位（repo `PlaceTree` 用法，`src/NpcGenerator.cpp:167, 181-182`）。
  - `TESDataHandler::CreateReferenceAtLocation(base, pos, rot, cell, worldspace, …, forcePersist, true)`
    （`T/TESDataHandler.h:72`）— 直接在指定座標+旋轉生，**更可控、適合一次擺一件到精準 transform**
    （repo `SpawnAtLocation` 用法，`src/NpcGenerator.cpp:48-52`）。城堡拼裝**首選這個**：每件給絕對座標+旋轉。
- **組件來源**：modular castle kit 的件是 `TESObjectSTAT`（`T/TESObjectSTAT.h:27`，FormType::Static）。
  vanilla 有現成的城堡/要塞 kit（如 Solitude/Whiterun 牆段、塔、門樓、稜堡）。用 EditorID 查最穩
  （repo 已示範 `LookupByEditorID<TESBoundObject>`，`src/NpcGenerator.cpp:128, 159`），免硬編
  FormID。`TESObjectSTAT` 是 `TESBoundObject`，可直接餵上面兩個 API。

### transform / snap 數學（複用 `MathUtil`）

- 每件 kit 在「結構本地座標系」有一組相對 transform `(localOffset, localRotZ)`。擺放時：
  1. 取結構原點 `origin`（世界座標）與結構朝向 `baseAngleZ`。
  2. 把 `localOffset` 繞 Z 旋轉 `baseAngleZ`（用 `MathUtil::Angle::RotateVector`，`src/util.h:283`，
     或直接 sin/cos 如 repo `src/NpcGenerator.cpp:36-39`）→ 加到 `origin` 得世界座標。
  3. 件的世界旋轉 = `baseAngleZ + localRotZ`（角度正規化用 `MathUtil::Angle::NormalAbsoluteAngle`，
     `src/util.h:247`）。
  4. 餵給 `CreateReferenceAtLocation(base, worldPos, NiPoint3{0,0,worldAngleZ}, …)`。
- **snap**：modular kit 的件以固定柵格設計（典型 wall 段長度為定值）。把 `localOffset` 量化到「件尺寸的
  整數倍」即可邊對邊接合。kit 件原點/接縫位置需逐 kit 校正（吃 NIF 的 pivot），這是**手調 recipe 的工作量**，
  非程式難點。`MathUtil::Clamp`（`src/util.h:202`）/ 量化函式輔助。

### 貼地與整平（交叉引用 NAVMESH 報告 §4(b)）

- **逐件貼地**：對每件的世界 (x,y)，先 `TES::GetLandHeight({x,y,0}, h)`（`T/TES.h:74`）拿地形 z；
  備援用 havok 向下射線（NAVMESH 報告 §4(b)：`bhkWorld::PickObject` / `TES::Pick`，吃到地形外的橋/岩石面）。
  把件的 z 設成 `h`（或 + 件的本地高度偏移）。
- **不平地面的整平策略**（因為**不能改地形**，見 §3）：
  - **單一基準面 + 地基填補**：取結構 footprint 內地形最高點 `zMax` 當基準平台高度；在低處用「地基/牆基/
    平台 static」（vanilla 有 foundation/stone block kit）疊高到 `zMax`，把城堡蓋在這個人造平台上。
  - 城堡主體件統一用 `zMax`（不逐件貼地），只有最外圈地基件逐件貼地往下填縫。這就是 vanilla 城市在
    斜坡上的做法（大量 retaining wall / 地基件），可在執行期模仿。
- **`SetScale`**（`TESObjectREFR::GetScale`，`:418`；`refScale` 在 `REFR_RUNTIME_DATA`，`:476`）可微調件大小填縫，
  但 kit 件多半不該縮放（破壞接縫），謹慎用。

---

## 3. 能不能執行期改地形 heightmap？— 幾乎不能（已查證）

- **landscape 高度資料在 `TESObjectLAND`**（`T/TESObjectLAND.h:43`，FormType::Land）。執行期高度在
  `LoadedLandData::heights[4][289]`（`:67`，4 象限各 17×17=289 頂點的浮點高度）。**這份資料確實在記憶體裡、
  理論上可讀可寫**。但：
  - **碰撞用的是烘焙好的 `hkpMoppCode moppCode`（`:76`）與 `BSTriShape geom[4]`（`:71`）**，不是
    每幀從 `heights` 重算。改 `heights` 不會改碰撞 mesh，也不會改可見幾何（要重建 `geom`/重編 mopp，
    SKSE 無公開 API）。
  - `OBJ_LAND::Flag::kVertexNormals_HeightMap`（`:21`）等說明 LAND 是「頂點法線+高度圖+圖層」的烘焙資料。
  - `BGSTerrainManager`（`B/BGSTerrainManager.h:5-41`）在 NG **整個是 `unkXX`，零具名 API**（唯一具名的是
    `lodTreesHidden`，`:16`）——沒有「重塑地形」的可呼叫入口。
- **結論**：**執行期不能實務地改地形 heightmap**（即使硬寫 `heights[]` 也只改數字、不改碰撞/視覺/LOD）。
  地形是烘焙 LAND 資料。**唯一務實的「整地」= §2 的擺地基/平台 static 視覺整平**，不是真的改地形。
  （repo `RaiseTerrain`/`LowerTerrain`，`src/NpcGenerator.cpp:119-152`，名為「terrain」實則只搬動物件 ref 的
  z，正印證此點 — 它動的是 ref 不是地形。）

---

## 4. 規模與 LOD

### 無遠景 LOD（最致命的視覺限制）

- **distant LOD 是預先烘焙的、綁在 base record 上**：`TESObjectSTAT::RecordFlags::kHasDistantLOD = 1 << 15`、
  `kHasTreeLOD = 1 << 6`、`kAddOnLODObject = 1 << 7`、`kUsesHDLODTexture = 1 << 17`
  （`T/TESObjectSTAT.h:46-52`）。這些 flag 表示「此 static 有對應的 LOD mesh/atlas」，但**那些 LOD 物件本身
  是 xLODGen/CK 預生成、塞進 worldspace 的 LOD 樹**，不是執行期可生成的。動態擺出來的 ref **不在任何
  LOD pass 裡**。
- **後果**：城堡只在 active grid（`uGridsToLoad`，預設 5×5）內的 full ref 才可見；
  - 玩家走出 grid → 整座城堡**消失**（不是降級成 LOD，是直接 detach 不渲染）。
  - 遠處天際線**沒有城堡剪影**（vanilla 城市遠看有，因為有烘焙 LOD）。
  - 走近時件**逐 cell pop-in**（cell attach 才建 3D）。
- **緩解（皆有限）**：
  1. **DynDOLOD「neverfade / Is-Full-LOD」手法**（見社群先例）：DynDOLOD 用「persistent + Is Full LOD flag
     的 neverfade ref」，並用 NearGrid/FarGrid 控制 enable/disable 來在更遠處顯示。但這需要**該物件本身有
     LOD 表示**且通常在 CK 端設定 — 純執行期擺的 STAT 不會自動取得 LOD，所以此手法對「動態城堡」**幫助有限**，
     只能延長 full-model 顯示距離（代價：效能指數上升）。
  2. **限制 footprint 在 3×3 cell 內**，讓整座城堡永遠在 active grid 裡（玩家在附近時），接受「遠看無剪影」。
  3. 把城堡放在玩家**會直接走到的近景**（地城入口、營地）而非遠眺地標。
- **誠實話**：要遠景剪影/無 pop-in 的大地標，**純執行期做不到**，那是 DynDOLOD/CK 預烘焙的領域。

### ref 數量 / 效能 / FormID 限制

- **FormID 池有限**：執行期建立的 ref 從 2^24 ≈ 1677 萬的 FormID 池取號（社群查證）。一座城堡幾百件
  尚遠不到上限，但**若「重複擺、不回收」會耗盡並導致存檔膨脹/CTD**（社群：腳本反覆 PlaceAtMe 是經典坑）。
- **persistent 與存檔膨脹**：`PlaceObjectAtMe`/`CreateReferenceAtLocation` 的 `forcePersist` 參數
  （`T/TESObjectREFR.h:461`、`T/TESDataHandler.h:72`）。**persistent ref 不會被引擎自動清理、會寫進存檔**。
  幾百件全 persistent → 存檔膨脹（社群實證 PlaceAtMe 大量 persistent 物件是 bloat 主因）。
- **緩解**：
  - **不要 forcePersist 城堡件**；改用 §5 的「存 recipe、讀檔重拼」，讓件本身是非持久（隨 cell detach 被清）。
  - 擺放分幀進行（每幀擺 N 件，用 `SKSE::GetTaskInterface()->AddTask` 排程），避免一次擺幾百件造成卡頓/CTD
    （repo 已知 init-CTD 教訓 → 操作要在主執行緒、且分散）。
  - 控制件數：用「實心牆段」而非「碎石堆」拼，件數可從上千降到幾百。

---

## 5. 持久化（規模比室內更嚴峻）

- **核心策略（與 `PROCGEN_INTERIOR.md` 的 persistence 段相同，這裡只述外景差異）**：
  **不持久化幾百個 dynamic ref，而是在 co-save 存一顆 seed + recipe id（+ 結構原點 transform），讀檔時重拼**。
  - 室內可能還能勉強 persist 少量裝飾 ref；**外景城堡件數太多，persist 幾乎必然存檔膨脹**，所以
    「存 recipe、讀檔重拼」對外景是**更硬性的要求**。
- **co-save 機制**：`SKSE::GetSerializationInterface()`（見 `COMMONLIBSSE_INDEX.md:33`；stub 簽名在
  `SKSE/Impl/Stubs.h:72-79`：`SetSaveCallback`/`SetLoadCallback`/`OpenRecord`/`WriteRecordData`/`ReadRecordData`）。
  存一筆小 record：`{ recipeId, seed, originWorldspaceFormID, originX, originY, originZ, baseAngleZ }`。
- **讀檔重拼流程**：
  1. load callback 讀回 recipe id + seed + origin。
  2. **不要在 load callback 裡直接擺**（cell 可能還沒 attach、主執行緒時機不對）。改記錄「待重拼結構」。
  3. 監看玩家進入目標 worldspace / 目標 cell **attach**（§1 的 `ICellAttachDetachEventSource`），
     在 attach 當下用同一 seed + recipe 重跑 §2 的擺放演算法 → **deterministic 重建同一座城堡**。
  4. 重拼的件**不 persist**（隨 cell detach 自然清掉，下次 attach 再拼）。
- **決定論要求**：擺放演算法對「同 seed + 同 recipe」必須**完全可重現**（與 `QUEST_ENGINE_SPEC.md` §8
  的確定性 random 精神一致）。任何隨機（牆裂縫、裝飾擺向）都走 seeded RNG，存的是 seed 不是結果。

---

## 6. 大型件的碰撞與「可走性」

- **碰撞**：`CreateReferenceAtLocation`/`PlaceObjectAtMe` 擺出的 STAT ref，其 3D 在 cell attach 時建立，
  **附帶 NIF 自帶的 havok 碰撞**（牆/塔的 collision mesh 隨件載入）。所以城堡牆**預設就擋玩家/物件**，
  不需額外處理。需要時可用 repo `NifUtil::Collision::ToggleMeshCollision`（`src/util.h:555`，
  `CFilter::Flag::kNoCollision`）開關某件碰撞。
- **keyframed vs havok**：城堡件是 static，擺好後不該被 havok 推動（vanilla static 是 fixed/keyframe 碰撞，
  不受重力）。`CreateReferenceAtLocation` 擺 STAT 即得此行為 — **不需要也不該**對結構件做 havok 動態 settle
  （那是給可動物件的，會掉/抖）。「settle」只發生在你錯用可動 base 時。
- **NPC 可走性（硬限制，交叉引用 NAVMESH 報告）**：
  - **拼出來的城堡沒有 navmesh**，引擎 NPC **無法在上面 pathing**（NAVMESH 報告 §1：navmesh 是烘焙
    `TESForm`，缺塊則 pathing 求解失敗、AI package 產不出移動指令）。
  - 要讓 NPC 在城堡裡/周邊走，**唯一務實路徑**是套用 NAVMESH 報告的「自驅 locomotion」：
    `EnableAI(false)` + 每幀自算方向 + `bhkCharacterController::SetLinearVelocityImpl` 推膠囊 +
    `GetLandHeight`/向下射線貼地（城堡地板會被射線命中，所以貼地對人造平台也成立）+ 餵
    `SpeedSampled`/`Direction` graph 變數播走路動畫。詳見 `NAVMESH_FREE_PATHFINDING.md` §2–§4。
  - 玩家本身用 char controller，**可以正常在城堡牆/階梯上走**（吃 havok 碰撞），所以「玩家可進可走」是
    成立的；只有 **NPC 自主移動**受限。

---

## 7. 分層判定

| 能力 | 執行期可行性 | 工作量／風險 | 誠實限制 |
|---|---|---|---|
| 在玩家周遭 attached 外部 cell 擺現成 STAT 件 | **可行**（repo 已驗證原語） | 低 | 限 active grid 內 |
| 用 MathUtil 做 snap/旋轉拼模組化城堡 | **可行** | 中（kit recipe 校正是手工活） | 接縫對齊吃 NIF pivot |
| `GetLandHeight`+射線貼地 / 擺地基整平 | **可行**（複用 NAVMESH §4） | 中 | 不平地形只能視覺整平 |
| 改地形 heightmap | **不可行** | — | LAND 烘焙、碰撞用 mopp、無 API |
| 遠景 LOD / 天際線剪影 / 無 pop-in | **不可行** | — | LOD 是預烘焙、動態 ref 不入 LOD pass |
| 大型結構跨 grid 邊界 | **半可行** | 高 | 需監 attach 分批擺、回收管理 |
| NPC 在城堡內 pathing | **不可行（直接）** | 高 | 無執行期 navmesh，須走自驅 locomotion（NAVMESH 報告） |
| persist 幾百件 | **不建議** | — | FormID 池/存檔膨脹 |
| 存 seed/recipe 讀檔重拼 | **可行（建議）** | 中 | 演算法須決定論 |

**總判定**：**「玩家會走近的近景小～中型結構（≤3×3 cell）」執行期完全可行且效果可信**；
**「遠眺大地標／需要 NPC 自主活動的活城市」純執行期做不到**（前者卡在 LOD，後者卡在 navmesh），
那些是 CK/xLODGen 預烘焙的領域。風險集中在：LOD pop-in（視覺）、navmesh 缺失（NPC）、存檔膨脹（持久化）、
以及大型結構跨 grid 的擺放/回收管理複雜度。

---

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
