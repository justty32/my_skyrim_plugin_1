# 程序化外景：LOD、持久化與可走性

[返回總索引](PROCGEN_EXTERIOR.md)

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
