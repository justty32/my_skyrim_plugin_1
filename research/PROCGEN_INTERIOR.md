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

## 0. 本 repo 已有的地基（直接複用，別重造）

`src/NpcGenerator.cpp` 已經把「執行期擺放既有 base object」這條路打通且驗證可運作，程序化室內就是把它**規模化**：

- **`SpawnAtLocation()`（`NpcGenerator.cpp:30-56`）**：示範了 `TESDataHandler::CreateReferenceAtLocation(base, pos, angle, cell, worldspace, nullptr, nullptr, ObjectRefHandle(), false, true)`——**精確座標+角度**擺放，正是擺模組化件需要的原語。
- **`PlaceTree()`（`NpcGenerator.cpp:154-191`）**：示範 `anchor->PlaceObjectAtMe(base, false)` + 之後 `SetPosition`/`SetAngle`，並用 `data.angle.z` 算 forward vector（`sin/cos`）把物件推到面前——擺房間錨點同一招。
- **`InitializeMagic()`（`NpcGenerator.cpp:241-310`）+ `SpellCastHandler`（行 195-239）**：動態造法術、按 `fullName` 前綴 `"C++: "` 分派——這就是後面「§7 最小範例：施法生成房間」要掛上去的骨架，照 `MODDING_COOKBOOK.md` R1 加一條 `else if` 即可。
- **form 查找慣例（`MODDING_COOKBOOK.md` §1.4）**：base 遊戲件用 `LookupByEditorID<T>("...")`（已在 `PlaceTree` 用，比硬編 FormID 穩），跨 mod 件用 `FormUtil::Parse`（`src/util.h:408-486`）。
- **`src/util.h` 數學/碰撞 helper**：`MathUtil::Angle::GetForwardVector/DegreeToRadian/ToRadianVector/RotateVector`（`util.h:227-293`）、`MathUtil::Interp::InterpTo`（行 332）、`ObjectUtil::Transform::InterpAngleTo`（行 363）、`NifUtil::Collision::ToggleMeshCollision`（行 555）、`NifUtil::Node::Clone`（行 491）。擺放/旋轉/碰撞切換**全部用現成的**。

**一句話**：程序化室內 = 把 `NpcGenerator` 的「擺一棵樹」變成「照一張食譜擺幾十件牆/家具/燈/雜物 + 收尾處理碰撞與存檔」。

---

## 1. 能不能執行期建立新的室內 CELL？——不行

**查證**：`class TESObjectCELL : public TESForm, public TESFullName`（`T/TESObjectCELL.h:113-116`），`FORMTYPE = FormType::Cell`（行 120）。CELL 是 form，烘在 load order；它的 `CreateDuplicateForm(...)` 直接 `{ return 0; }`（行 181，註解寫明）。`IFormFactory::GetConcreteFormFactoryByType<T>()`（`I/IFormFactory.h:40`）走的是 `ConcreteFormFactory<T, T::FORMTYPE>`——`NpcGenerator` 拿它造 `TESNPC`/`SpellItem`/`EffectSetting` 沒問題，但 CELL 不是設計成執行期可造的 form：它需要 `INTERIOR_DATA`（光照/霧）、portal graph、navmesh array、`LOADED_CELL_DATA` 等一整套烘焙資料（見 `RUNTIME_DATA_CONTENT`，行 224-239）。**沒有可呼叫的「建立並掛載一個新 CELL」C++ 入口。**

**務實模型（二選一，可混用）**：

| 模型 | 載體 CELL | 優點 | 缺點 |
|---|---|---|---|
| **A. 既有空室內** | 預先存在的空 interior（vanilla 有不少幾乎空的小室內，或玩家進到某個房間） | 有正確的室內光照/portal/天花板包覆、不漏天、navmesh 已存在 | 不能保證「乾淨空房」尺寸正好；房間形狀被既有牆限制 |
| **B. 玩家周遭 worldspace / 當前 CELL** | 玩家當前所在 CELL（`anchor->GetParentCell()`，見 `NpcGenerator.cpp:50`） | 自由度最高、想擺哪擺哪、`NpcGenerator` 已驗證可在此擺件 | 露天無天花板（要自己擺屋頂件）、室外光照、**新區域無 navmesh → NPC 走不動** |

**實作含義**：兩種模型都用同一組擺放原語（§2-§3）。差別只在「要不要自己擺天花板/外牆把空間包起來」與「光照來源」（§6）。模型 B 更貼近本 repo「在玩家面前生成」的既有風格（`NpcGenerator` 全是模型 B）。

---

## 2. 擺殼（模組化靜態件）

### 2.1 用哪些 base form

- **`TESObjectSTAT`**（`T/TESObjectSTAT.h:27`，`FORMTYPE = FormType::Static`）：牆、地板、天花板、房間段、門框等模組化「建築積木」的主要型別。vanilla 的 modular kit（如 `WRTemple*`、`MRH*`/Markarth、`NorRoom*`/Nordic ruins、`FarmHouse*`）全是 STAT。用 `LookupByEditorID<TESObjectSTAT>("...")` 取得，再當 `TESBoundObject*` 餵給擺放原語（`TESObjectSTAT : TESBoundObject`，行 28）。
- **`BGSMovableStatic`**（`B/BGSMovableStatic.h:24`，`FORMTYPE = FormType::MovableStatic`，繼承 `TESObjectSTAT`）：可被腳本移動/動畫的靜態件。對「之後還想搬動整面牆」有用，否則一般用純 STAT 即可。
- 兩者都是 `TESBoundObject`，所以 **`PlaceObjectAtMe` / `CreateReferenceAtLocation` 直接吃**，與 `NpcGenerator` 擺樹/石完全同一條路。

### 2.2 擺放與旋轉原語（已驗證）

- **精確座標**：`TESDataHandler::CreateReferenceAtLocation(base, pos, rot, cell, worldspace, nullptr, nullptr, ObjectRefHandle(), forcePersist, true)`（`T/TESDataHandler.h:72`；用法見 `NpcGenerator.cpp:48`）。`pos`/`rot` 都是 `NiPoint3`，**rot 是弧度**（x/y/z 尤拉角）。模組化件需要「擺在哪、轉幾度」，這是首選——可以一次給齊位置與朝向。
- **先擺再調**：`anchor->PlaceObjectAtMe(base, false)` 回 `NiPointer<TESObjectREFR>`（`T/TESObjectREFR.h:461`），再 `SetPosition(NiPoint3)`（行 470）、`SetAngle`（透過寫 `data.angle` 或用既有 `NpcGenerator` 的 `spawned->SetAngle(...)` 慣例，見 `NpcGenerator.cpp:93,182`）。
- 角度數學一律用 `src/util.h`：`MathUtil::Angle::DegreeToRadian`（食譜常用「度」描述，引擎吃「弧度」）、`ToRadianVector(x,y,z)`（行 237，整支尤拉角一次轉）、`GetForwardVector`/`RotateVector`（行 283-293，把「相對某錨點往前 N 單位」算出來）。

### 2.3 模組化拼接 / snap 的座標數學

vanilla modular kit 是設計成「同一族件對齊到固定格點就能無縫拼接」。執行期沒有 CK 的 snap node 自動吸附，但可以**用數學模擬**：

1. **房間錨點（room anchor）**：選一個原點 `O`（玩家面前 N 單位 + 一個 yaw `θ`，沿用 `NpcGenerator` 的 forward-vector 推法），所有件的座標都以 `O` 為基準的「相對 offset」描述（食譜給相對值，執行期轉世界座標）。
2. **相對 → 世界座標**：`worldPos = O + Rz(θ) · localOffset`，其中 `Rz(θ)` 用 `MathUtil::Angle::RotateVector(localOffset, quatFromYaw)` 或自己用 `sin/cos` 套（`NpcGenerator.cpp:36-39` 已示範 z-yaw 的 sin/cos）。件的朝向 = `localYaw + θ`。
3. **格點對齊（grid snap）**：modular kit 的件多半是固定尺寸（常見 256/512 單位的倍數）。食譜把 offset 表成「格數 × 格距」，執行期乘出來即可保證鄰件邊緣對齊。**格距值要按所選 kit 量測填進食譜**（同一族件量一次即可）。
4. **包覆順序**：先擺地板格 → 再沿邊擺牆 → 角落擺轉角件 → 上方擺天花板/屋頂件。模型 B（露天）一定要擺天花板，否則漏天、室外光直射。

> 註：執行期擺放接縫不會像 CK 手工微調那麼完美；對「程序化」用途可接受。要更準可在食譜裡微調個別 offset，或之後做「相鄰件 AABB 對齊」的後處理（成本較高，非 MVP 必要）。

---

## 3. 家具與裝飾

### 3.1 家具（讓 NPC/玩家能坐/睡/用）— 關鍵：標記隨模型走，不靠 CELL

- **`TESFurniture`**（`T/TESFurniture.h:10`，`TESFurniture : TESObjectACTI`，`FORMTYPE = FormType::Furniture`）：椅子、床、長凳、工作台等。`ActiveMarker` flags（行 17-51）標明能 `kCanSit`/`kCanSleep`/`kCanLean`/`kIsPerch`，以及 24 個 `kSit0..kSit23` 槽位；`WorkBenchData`（行 68-87）標明是不是煉金/打鐵/附魔台。它是 `TESBoundObject`（經 `TESObjectACTI`），**一樣用 `PlaceObjectAtMe`/`CreateReferenceAtLocation` 擺**。
- **為什麼擺下去 NPC/玩家就能用**：家具的「坐/睡 marker」是 **`BSFurnitureMarker`**（`B/BSFurnitureMarkerNode.h:9-32`：`offset`/`heading`/`animationType`(Sit/Sleep/Lean)/`entryProperties`），整包 `BSFurnitureMarkerNode : NiExtraData`（行 34-41）**掛在家具的 NIF 模型上**。也就是說——marker 隨 base 模型，不是 CELL 烘焙的東西。**所以執行期 `PlaceObjectAtMe` 一張椅子，它的坐標記就跟著生效**，玩家/NPC 走過去就能用 `Activate`（`TESFurniture::Activate` vfunc 0x37，`T/TESFurniture.h:125`）坐下。`Actor::GetSitSleepState()`（`A/ActorState.h:159`）可查使用狀態。
- **NPC 用家具的 caveat（重要）**：玩家走過去自己坐沒問題；但要**NPC 主動走去用**家具（AI package 的 Sit/Sleep/UseFurniture），NPC 得先**能走到那把椅子**——而程序化新區域**沒有 navmesh，NPC pathing 求解失敗就到不了**（見 `NAVMESH_FREE_PATHFINDING.md` §1）。緩解：把 NPC 直接 `MoveTo`/`SetPosition` 放到家具旁，或用該文件的自製 steering 把 NPC 推到位再讓它 `Activate`。**「擺一個能用的家具」可行；「讓 NPC 自己跨房間走去用」要配合無 navmesh 移動方案。**

### 3.2 idle markers / 雜物 / 容器

- **idle marker**：vanilla 的 `IdleMarker`（一種 STAT/ACTI marker）讓 NPC 播 idle 動畫；同樣是 base object，可擺。讓 NPC 真的去用同樣受 navmesh 限制。
- **雜物（clutter）**：盤子、書、瓶子、食物等多為 `TESObjectMISC` / `TESObjectSTAT` / `TESObjectBOOK` 等 `TESBoundObject`，全部 `PlaceObjectAtMe` 即可擺到桌面上。要不要讓它有物理見 §4。
- **容器**：`TESObjectCONT`（箱、櫃）是 `TESBoundObject`，擺下去就是可開的容器；要放東西進去用 `AddObjectToContainer`（`MODDING_COOKBOOK.md` R6 同款）。

### 3.3 燈

- **`TESObjectLIGH`**（`T/TESObjectLIGH.h:56`，`FORMTYPE = FormType::Light`）：`OBJ_LIGH data`（行 104）含 `radius`/`color`/`fov`/`fallofExponent`/`flags`（`TES_LIGHT_FLAGS`，行 18-37，含 `kSpotlight`/`kOmniShadow`/`kFlicker` 等）。是 `TESBoundObject`（經 `TESBoundAnimObject`），**可擺**。
- 兩類：① 帶模型的光源件（火把/壁燈/吊燭，自帶網格+發光，最省事，直接擺一個就有光有模型）；② 純光（無模型 light ref，補環境光）。模型 B（露天）室內偏暗時尤其需要補幾盞。**注意 §6**：執行期擺的 light ref 是否被既有 CELL 的 portal/roombound 正確接受、會不會「整間亮」而非分區，是要實測的點。

### 3.4 裝飾佈局策略

食譜驅動，三種由簡到繁：

1. **grid（網格）**：地板/牆用固定格距鋪滿（§2.3）。最適合「殼」。
2. **anchor-point（錨點插槽）**：殼擺好後，在食譜裡對每個「家具插槽」給一個相對 transform（如「北牆中央朝南放一張床」）。最適合家具/大件。
3. **template-driven（模板/藍圖）**：整間房 = 一份 JSON 藍圖（殼件清單 + 家具插槽 + 燈 + 雜物散佈規則），見 §8。雜物可在桌面 AABB 內用 seed 亂數散佈（程序化感）。

---

## 4. 雜物的物理落定（havok settle）

### 4.1 兩種擺法

| 做法 | 機制 | 穩定度 |
|---|---|---|
| **keyframed / fixed 擺放（推薦預設）** | 擺好就 `SetMotionType(MotionType::kKeyframed)` 或 `kFixed`（`T/TESObjectREFR.h:468` `SetMotionType(MotionType, bool)`；enum `T/TESObjectREFR.h:126-135`，`kKeyframed=4`/`kFixed=5`/`kDynamic=1`）。物件釘死在算好的位置，不參與模擬。 | **最穩**，無抖動/穿模，可重現 |
| **dynamic 落定（settle）** | 擺在桌面略上方，`SetMotionType(MotionType::kDynamic)`（=1），讓 havok 重力把它落到桌面停住 | 自然、有變化，但**抖動/穿桌/滾落/效能**風險，要實測 |

### 4.2 執行期可用的 havok 原語（已查證）

- **ref 層**：`TESObjectREFR::SetMotionType(MotionType, bool a_allowActivate=true)`（`T/TESObjectREFR.h:468`）切動態/keyframe/fixed；`MoveHavok(bool)`（vfunc 0x85，行 333）把 havok 表示同步到目前座標；`InitHavok()`（vfunc 0x66，行 298）；`SetCollision(bool)`（行 465）/`HasCollision()`（行 426）開關碰撞。
- **rigid body 層**：`bhkRigidBody`（`B/bhkRigidBody.h:13`）有 `SetPositionAndRotation`（vfunc 0x37，行 45）、`SetLinearVelocity`/`SetLinearImpulse`/`SetAngularVelocity`（行 52-55）——要「把雜物推一把讓它落」可用，但一般不需要。
- **mesh 碰撞開關（repo 既有）**：`NifUtil::Collision::ToggleMeshCollision(root, bhkWorld, state)`（`src/util.h:555`，走 `BSVisit::TraverseScenegraphCollision` 設 `CFilter::Flag::kNoCollision`）。可在「擺放/搬動殼件時暫時關碰撞避免互相彈飛」，擺定後再開回。
- `bhkWorld` 由 `TESObjectCELL::GetbhkWorld()`（`T/TESObjectCELL.h:197`）取（`NAVMESH_FREE_PATHFINDING.md` 也用同一支做射線）。

### 4.3 建議

- **殼件（牆/地板/天花板）一律 `kFixed`**：它們本來就該不動，且彼此緊貼，動態化只會互相彈飛。
- **雜物預設 `kKeyframed`/擺準即可**；只有想要「散亂自然感」時，對少量小件開 `kDynamic` 落定，且**先 PoC 驗抖動/穿模**。落定一兩秒後可再切回 `kKeyframed` 凍結，避免長期占用模擬。
- **最該先驗的單一未知數**：在目標 CELL 的 `bhkWorld` 裡，一批雜物 `kDynamic` 同時落定會不會互穿/抖/掉穿桌面。先用 3-5 件做最小 PoC。

---

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

---

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
