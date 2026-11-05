# 程序化室內：地基、CELL 限制、擺殼、家具與物理落定

[返回總索引](PROCGEN_INTERIOR.md)

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
