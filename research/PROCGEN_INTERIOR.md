# 執行期程序化生成室內空間（含家具／裝飾擺放）— SKSE C++ 可行性與設計分析

> 探索性可行性分析，2026-05-23。由 research agent 產出。所有引擎面論斷均對照本 repo vendored 的
> `CommonLibSSE-NG/include/RE/` 標頭逐一查證並標出真實路徑/行號。**僅分析，未動 plugin 程式碼**
> （`src/`、`cmake/`、`plugin.cpp` 由其他 worktree 並行編輯，本報告一律不碰）。
> 與本專案 ethos 一致（`CLAUDE.md`、`MODDING_COOKBOOK.md` §1.1、memory `project_quest_engine`）：
> **不開 Creation Kit、不做 ESP/ESM，全部在執行期用程式碼擺放既有 base object。**
> 交叉引用：navmesh 不可執行期生成已在 `research/NAVMESH_FREE_PATHFINDING.md` 詳述，本報告沿用其結論。

---

## 結論先講（TL;DR）

- **可行**：在「一個既有的空（或夠空的）室內 CELL」或「玩家周遭的 worldspace」裡，用本 repo 已驗證的
  `TESObjectREFR::PlaceObjectAtMe` / `TESDataHandler::CreateReferenceAtLocation` 把模組化牆/地板/房間靜態件
  （`TESObjectSTAT` / `BGSMovableStatic`）、家具（`TESFurniture`）、燈（`TESObjectLIGH`）、雜物
  逐件擺進去，靠座標 + 角度數學拼出一個房間。家具能讓 NPC/玩家「坐/睡/用」，因為**家具標記
  （`BSFurnitureMarker`）是綁在家具的 NIF 模型上**、隨擺放的 ref 一起生效，不依賴 CELL 烘焙。
- **不可行／硬限制**：
  1. **執行期建立新的 CELL 不可行**——`TESObjectCELL` 是 `TESForm`（`T/TESObjectCELL.h:113`），
     烘焙在 load order 裡，`CreateDuplicateForm` 直接 `{ return 0; }`（行 181）。沒有 `IFormFactory` 路徑造它。
  2. **roombound / portal / 室內光照分區是 CELL 烘焙的，不可執行期生成**——portal graph（`BSPortalGraph`）
     掛在 `LOADED_CELL_DATA::portalGraph`（`T/TESObjectCELL.h:83`）與 `TESWorldSpace::portalGraph`
     （`T/TESWorldSpace.h:200`），由 CELL 的 RoomMarker/PortalMarker 資料烘成；沒有型別化 API 執行期建構。
  3. **navmesh 不可執行期生成**（見 `NAVMESH_FREE_PATHFINDING.md`）——生成的房間裡 vanilla NPC **無法用引擎
     pathing 走動**，這是程序化室內最大的活物限制。
- **最大未知數（要先 PoC）**：① 雜物丟下去靠 havok「落定（settle）」在執行期到底穩不穩、會不會抖/穿；
  ② 大量動態 ref 跨 save/reload 的存活與效能。兩者都建議走「**存種子+食譜、讀檔重建**」而非「相信引擎幫你存幾百個 dynamic ref」。

---

本分析已按主題拆分：

- [地基、CELL 限制、擺殼、家具與物理落定](PROCGEN_INTERIOR_CELL_LAYOUT.md)：既有地基、新 CELL 不可行、模組化殼與 snap、家具／燈／裝飾、havok settle。
- [持久化與光照硬限制](PROCGEN_INTERIOR_PERSISTENCE_LIGHTING.md)：save/reload 存活策略、SKSE co-save、portal／roombound／光照限制。
- [分層判定、最小範例、JSON adapter 與先例](PROCGEN_INTERIOR_IMPLEMENTATION.md)：分層可行性、施法生成小屋 spec、`generate_interior` adapter、先例與引用標頭。
