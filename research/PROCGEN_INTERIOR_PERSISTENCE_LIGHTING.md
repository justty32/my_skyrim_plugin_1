# 程序化室內：持久化與光照硬限制

[返回總索引](PROCGEN_INTERIOR.md)

## 5. 持久化（save/reload 存活）

### 5.1 引擎面事實（已查證）

- **temporary vs persistent ref**：`PlaceObjectAtMe(base, a_forcePersist)`（`T/TESObjectREFR.h:461`）與 `CreateReferenceAtLocation(..., a_forcePersist, ...)`（`T/TESDataHandler.h:72`）的布林參數決定持久性。`NpcGenerator` 全傳 `false`（temporary，`NpcGenerator.cpp:51,78,167`）。`IsPersistent()`（`T/TESObjectREFR.h:453`）可查。
- **生命週期 API**：`Disable()`（vfunc 0x89，`T/TESObjectREFR.h:337`）/ `Enable(bool)`（行 374）/ `IsDisabled()`（行 442）；`SetDelete(bool)`（vfunc 0x23，行 234）標記刪除、`IsMarkedForDeletion()`（行 451）。`CreateRefHandle()`/`GetHandle()`（行 371,397）拿 handle、`LookupByHandle`（行 363）查回——這是管理動態 ref 的 handle 機制（`NpcGenerator::GetCrosshairTarget` 已用 `crosshair->target[0].get()` 這類 handle 取值）。
- **ref-count / handle 問題**：dynamic（FF 開頭 FormID）ref 由引擎以 handle 計數管理；持有裸指標跨幀/跨存檔不安全，要存就存 `ObjectRefHandle` 或自己的食譜，不要存 `TESObjectREFR*`。

### 5.2 兩種持久化策略

| 策略 | 做法 | 評價 |
|---|---|---|
| **(A) 靠引擎存 persistent ref** | 擺放時 `forcePersist=true`，讓幾十～幾百個 ref 進存檔 | 簡單，但**幾百個動態 persistent ref 會脹存檔、拖效能、且 dynamic FormID 重載後不保證穩**。不建議用於整間房。 |
| **(B) 存「種子 + 食譜」，讀檔重建（推薦）** | 存檔時用 SKSE co-save 寫入：用哪份房間模板 ID、錨點座標/朝向、亂數 seed、玩家做過的增刪改 delta；讀檔時把舊 ref 清掉、依食譜+seed**重新擺一次**。擺出來的 ref 全用 `forcePersist=false`（temporary） | **本專案首選**。存檔只存一小塊配方，重現性由 seed 保證，與本 repo「動態造的東西不進存檔、讀檔重建」慣例一致（`MODDING_COOKBOOK.md` §1.2：動態法術 `kNewGame`/`kPostLoadGame` 都要重給） |

### 5.3 SKSE co-save 機制（已查證）

`SKSE::SerializationInterface`（`SKSE/Interfaces.h:81`）：`SetSaveCallback`/`SetLoadCallback`（行 97-99）、`OpenRecord(type,version)`（行 125）、`WriteRecord`/`WriteRecordData`（行 101,127）、`ReadRecordData`（行 153）。把「房間食譜 ID + 錨點 transform + seed + 玩家 delta」序列化進 co-save，`SetLoadCallback` 裡讀回並重建。**這正是 `QUEST_ENGINE_SPEC.md` §6.2「進度 blob 寫進當前存檔」在 Skyrim adapter 上落地的同一條路（spec 第 230 行明指 SKSE co-save）。** 程序化室內的存檔可直接複用 quest engine 的 co-save 層。

---

## 6. 光照 / portal / roombound — 硬限制

**查證**：

- **portal graph 是 CELL/worldspace 烘焙的**：`LOADED_CELL_DATA::portalGraph`（`NiPointer<BSPortalGraph>`，`T/TESObjectCELL.h:83`）、`TESWorldSpace::portalGraph`（`T/TESWorldSpace.h:200`）、`BSLight::portalGraph`（`B/BSLight.h:71`）。`BSPortalGraph`（`B/BSPortalGraph.h:12`）只有解構子與 size，**無「執行期建 portal/room」型別化 API**。
- **roombound = RoomMarker/PortalMarker** 是 CELL 內烘焙的特殊 marker（CK 概念），由它們生成 portal graph 做遮蔽剔除與光照分區。`BSMultiBound*`（`B/BSMultiBound.h` 等）/`BSMultiBoundRoom`（`B/BSMultiBoundNode.h:50` 的 `GetMultiBoundRoom`）是它的執行期表示，同樣**只有查詢式 API，無建構式**。
- **室內光照分區**：`INTERIOR_DATA`（`I/InteriorData.h:8`：ambient/directional/fog/lightFade…）掛在 `TESObjectCELL`，由 CELL 與 `BGSLightingTemplate`（`T/TESObjectCELL.h:238`）決定。執行期 `SetFogColor`/`SetFogPlanes`/`SetFogPower`（`T/TESObjectCELL.h:214-216`）能**改既有 CELL 的霧/光氛圍**，但不能新建一個帶獨立 roombound 的分區。

**結論**：

- **portal/roombound/光照分區無法執行期生成**——這是硬限制。
- **務實做法**：用**模型 A（既有空室內）**，直接繼承該 CELL 既有的 portal/roombound/室內光照分區（最省事、視覺最正確）；你只負責往裡擺件。
- 模型 B（露天）沒有室內 roombound，**靠多擺 `TESObjectLIGH` light ref 補光**即可達到「看起來是個有照明的房間」，但拿不到正確的遮蔽剔除分區（遠處看得到牆內燈），對 MVP 可接受。
- 可用 `TESObjectCELL::GetbhkWorld()`/`GetLighting()`（`T/TESObjectCELL.h:197,202`）讀既有環境，**改氛圍可以，建分區不行**。
