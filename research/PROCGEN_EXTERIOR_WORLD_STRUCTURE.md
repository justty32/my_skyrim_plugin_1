# 程序化外景：Worldspace、結構拼裝與地形限制

[返回總索引](PROCGEN_EXTERIOR.md)

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
