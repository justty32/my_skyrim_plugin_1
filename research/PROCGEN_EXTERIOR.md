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

本分析已按主題拆分：

- [Worldspace、結構拼裝與地形限制](PROCGEN_EXTERIOR_WORLD_STRUCTURE.md)：可擺放範圍、模組化 static 組件、貼地與 heightmap 邊界。
- [LOD、持久化與可走性](PROCGEN_EXTERIOR_LIMITS_PERSISTENCE.md)：規模、LOD、co-save recipe、碰撞與 navmesh 限制。
- [最小實作、JSON adapter 與查證來源](PROCGEN_EXTERIOR_IMPLEMENTATION.md)：spell 規格、`generate_structure`、社群先例與標頭索引。
