# 不靠 navmesh 讓 NPC 在簡單地形上尋路移動 — 可行性／設計分析

> 探索性可行性分析，2026-05-23。由 research agent 產出，所有引擎面論斷均對照本 repo vendored 的
> `CommonLibSSE-NG/include/RE/` 標頭查證。**僅分析，未動 plugin 程式碼。**
> 本報告只談**位移與尋路層（locomotion + pathfinding）**；AI package / behavior tree 由
> `NPC_BEHAVIOR_DEEP_MODIFICATION.md` 負責，這裡只在「讓走路動畫真的播、不滑步」時引用到。

## 結論先講（TL;DR）

- **可行**：自寫 steering（朝目標走直線 + 前方障礙射線探測）+ 用 `RE::TES::GetLandHeight` / havok 向下射線做地面貼合 + 餵動畫圖變數（`SpeedSampled`/`Direction`/`bAnimationDriven`）讓走路動畫播出來，配合每幀推進位置（用 repo 既有 `MathUtil::Interp::InterpTo` 做 frame-rate 無關）。對「不太複雜的平地」完全做得到。
- **不可行／別碰**：從 SKSE 動態「生成 navmesh」基本不可能——`NavMesh` 是 `TESForm`；navmesh 求解器（`BSPathingSystem`/`PathingRequest`/`BSPathingSolution`）在 NG 裡**只有 RTTI/VTABLE offset，沒有可呼叫的 C++ 類別宣告**，無法用型別化 API 餵自製路徑給引擎 pathing；只能整套繞過、自己驅動角色。
- **最大未知數（要先驗證）**：把走路動畫切到 animation-driven 後，角色的**碰撞膠囊（`bhkCharacterController`）會不會跟著真的位移**，還是只有骨架原地擺動、膠囊不動（即「滑步／原地踏步」）。這是成敗關鍵，必須最先用最小 PoC 驗。

---

## 1. 引擎正常怎麼尋路與移動，為何離開 navmesh 就失敗

**移動控制器鏈**
- `RE::Actor` 持有 `BSTSmartPointer<MovementControllerNPC> movementController`（`A/Actor.h:667`，`ACTOR_RUNTIME_DATA` 內 offset 0x148）。
- `MovementControllerNPC : MovementControllerAI, IMovementMessageInterface, IMovementMotionDrivenControl, IMovementSelectIdle, IMovementDirectControl, IMovementPlannerDirectControl, IAnimationSetCallbackFunctor`（`M/MovementControllerNPC.h`）。但**所有成員都是 `unkXXX`、所有 vfunc 都是 `Unk_0A…Unk_14`**——沒有任何具名 API。
- 它繼承的 `IMovement*` 介面（`I/IMovementDirectControl.h`、`I/IMovementMessageInterface.h` 等）**全是 `Unk_01…` 空殼**。引擎的「導航式移動」入口在 NG 裡是黑盒。

**Pathing 系統根本沒被 vendored**
- grep `BSPathingSystem`/`Pathing` 類別只命中 `Offsets_RTTI.h` 與 `Offsets_VTABLE.h`：`RTTI_BSPathingSolution`、`RTTI_PathingRequest`、`RTTI_PathingRequestClosePoint`、`RTTI_PathingRequestSafeStraightLine`（`Offsets_RTTI.h:1698-1705`），`VTABLE_MovementMessageNewPath`、`VTABLE_MovementMessageWarpToLocation`（`Offsets_VTABLE.h:6218, 6276`）。
- **意義**：這些在 binary 裡存在，但 CommonLibSSE-NG 沒給 `class` 宣告。要呼叫只能裸位址 + 自己逆向 layout，風險極高。「用引擎 pathing 算路、再餵自製路徑」在此 SDK 上不成立。

**navmesh 形態**：`RE::NavMesh : TESForm, TESChildCell, BSNavmesh`（`N/NavMesh.h:10-13`）——navmesh 是 **form/資料**，不是執行期可即時重建的幾何。另有 `B/BSNavmesh.h`、`B/BSNavmeshInfoMap.h`、`N/NavMeshInfoMap.h`、`B/BSPrecomputedNavmeshInfoPathMap.h`（名字就點明 precomputed）。「執行期動態產生 navmesh」不切實際。

**為何離開 navmesh 角色就不走**：引擎 NPC 移動是「先用 navmesh 求路 → 沿路點推進」。沒有 navmesh（哪怕缺一小塊），pathing 求解失敗，AI package 就**無法產生移動指令**（CK wiki：navmesh 上沒有 green triangle，actor 視同身處另一個太陽系）。問題不在「actor 拒絕走路」這個 flag，而在**上游 pathing 求解器交不出路徑**，於是 movement controller 收不到任何 `MovementMessageNewPath`。

**推論**：唯一務實做法是**整層繞過引擎 pathing，自己當「移動控制器」**——自己算方向、推位置、驅動動畫。

---

## 2. SKSE 能直接呼叫的位移原語（會不會播動畫／吃不吃碰撞重力）

| 原語 | 標頭 | 行為 | 動畫？ | 碰撞／重力？ |
|---|---|---|---|---|
| `TESObjectREFR::SetPosition(NiPoint3)` | `T/TESObjectREFR.h:469-470` | 直接寫座標 | 否 | 瞬移，易穿牆 |
| `Actor::SetPosition(const NiPoint3&, bool a_updateCharController)`（vfunc 0xA9） | `A/Actor.h:367` | actor 版，可選擇是否更新 char controller | 否 | `true` 時同步膠囊 |
| `TESObjectREFR::MoveTo(target)` / `MoveTo_Impl` | `T/TESObjectREFR.h:456, 507` | 瞬移到另一 refr（≈ console `moveto`） | 否 | 瞬移、會 reset |
| `Actor::Move(float, const NiPoint3&)`（vfunc 0xC8，回 `bhkCharacterController*`） | `A/Actor.h:398` | **每幀位移增量**，走 char controller | 否 | **是**——引擎自己每幀推 actor 用，吃碰撞 |
| `ObjectUtil::Transform::TranslateTo(...)`（`RELOCATION_ID(55706, 56237)`，repo 已有） | `src/util.h:356` | 包裝 Papyrus `TranslateTo`，平滑移到座標+角度 | 否（純 keyframe） | 對 actor：社群實測「除了地面，會穿過所有東西」，**滑步嚴重** |
| `bhkCharacterController::SetLinearVelocityImpl(const hkVector4&)`（vfunc 0x07） | `B/bhkCharacterController.h:78`；proxy 版 `B/bhkCharProxyController.h:29` | 直接設膠囊線速度 | 否 | **是，最物理正確**——havok char proxy 帶碰撞、被地面/牆擋、吃重力 |
| `Actor::GetCharController()` | `A/Actor.h:531` | 取得上面那個 controller | — | — |
| `KeepOffsetFromActor` | **未 vendored**（grep 在 `Actor.h`/`AIProcess.h` 皆無，只 Papyrus 層有）| 維持相對位移，**仍走 navmesh pathing** | 是（引擎自走） | 是——但**依賴 navmesh**，離開即失效，對本題無用 |

**關鍵分辨**
- 上面**所有原語都只負責「把碰撞膠囊搬過去」，沒有一個自帶走路動畫**。直接用任何一個都會「人形以 idle 姿勢平移滑過去」（滑步）。
- 真正「物理正確且吃碰撞」的是 **`bhkCharacterController::SetLinearVelocityImpl`**（havok char proxy，`B/bhkCharProxyController.h`，proxy 本身在 `H/hkpCharacterProxy.h`）與 **`Actor::Move`（0xC8）**。其餘（`SetPosition`/`MoveTo`/`TranslateTo`）是 keyframe/瞬移性質。
- 位移要靠這兩者其一；**動畫要另外驅動**（見 §3）。

---

## 3. 讓位移「看起來像真的在走」——驅動動畫圖

`RE::Actor`（經 `Character : TESObjectREFR`）繼承 `IAnimationGraphManagerHolder`（`T/TESObjectREFR.h:112`，offset 0x38）。可用 API（`I/IAnimationGraphManagerHolder.h:44-54`）：`SetGraphVariableFloat/Bool/Int(name, v)`、`NotifyAnimationGraph(eventName)`（vfunc 0x01）、對應 `GetGraphVariable*`。

**引擎實際使用的變數名**（由 `F/FixedStrings.h` 確認，非猜）：
- `"SpeedSampled"`（0x520）、`"Speed"`（0x508）、`"Direction"`（0x518）、`"MovementDirection"`（0x498）、`"TurnDelta"`（0x510）、`"VelocityZ"`（0x500）——humanoid behavior graph 控制走/跑混合與轉向的核心輸入。
- `"bAnimationDriven"`（0x568）、`"bAllowRotation"`（0x570）、`"bIsSynced"`/`"bSpeedSynced"`（0x578/0x580）。
- tween 子系統：`"bTweenUpdate"`、`"TweenSpeed"`/`"HasTweenSpeed"`、`"TargetLocation"`、`"pathTweenerStart"`/`"pathTweenerEnd"`（0x488–0x5E8）——引擎沿 navmesh 路點段間平滑移動所用的 path tweener。

**兩種驅動模式（必須分清）**
1. **物理位移 + 驅動走路動畫（推薦）**：每幀用 §2 `SetLinearVelocityImpl`（或 `Actor::Move`）把膠囊往目標推；同時 `SetGraphVariableFloat("SpeedSampled", v)`、`SetGraphVariableFloat("Direction", θ)` 讓 graph 播對應速度/方向動畫；用 repo 既有 `ObjectUtil::Transform::InterpAngleTo`（`src/util.h:363`）平滑轉身。動畫是「裝飾」，真正位移由速度寫入決定。**只要速度數值與動畫播放速度大致匹配，腳就不明顯滑**（這也是 vanilla NPC 本質：motion-driven）。
2. **純 animation-driven（root motion 帶位移）**：`SetGraphVariableBool("bAnimationDriven", true)`（社群實證開關，等同 Papyrus `SetAnimationVariableBool`），讓**動畫 root motion 直接驅動位移**；切回 `false` + `bMotionDriven=true` 還原。問題：原版 locomotion 是 in-place（原地擺動靠 motion 推進），要靠 root motion 帶走需要 root-motion 走路動畫（OAR/DAR/AMR 那套），方向受動畫限制，不如模式 1 可控。**Animation Motion Revolution (AMR)** 這類 mod 正是讓引擎正確讀動畫 root-motion 位移、消滑步——印證 animation-driven 位移可行但需動畫端配合且引擎讀取有坑。

**取捨**：模式 1（自推速度 + 餵 `SpeedSampled`/`Direction`）對「不複雜地形」最穩、最可控，不需自帶 root-motion 動畫資產。模式 2 留給「想極致無滑步」時再加。

---

## 4. 自製尋路（不靠 navmesh）— 設計選項

### (a) 直線 steering + 射線探測（推薦核心）
- **朝目標走直線**：`dir = normalize(target - actorPos)`，用 `MathUtil::Angle::GetAngle`/`GetForwardVector`（`src/util.h:272, 291`）算朝向，`InterpAngleTo` 平滑轉身。
- **前方障礙探測**：havok 射線。`RE::TESObjectCELL::GetbhkWorld()`（`T/TESObjectCELL.h:197`）取 `bhkWorld`；`bhkWorld::PickObject(bhkPickData&)`（vfunc 0x33，`B/bhkWorld.h:37`）做射線；`bhkPickData`（`B/bhkPickData.h`）含 `hkpWorldRayCastInput rayInput`（`from`/`to`/`filterInfo`）與 `hkpWorldRayCastOutput rayOutput`（`HasHit()`、`rootCollidable`）；或更高階 `RE::TES::Pick(bhkPickData&)`（`T/TES.h:77`）。注意 havok 與遊戲座標差一個 `bhkWorld::GetWorldScale()`（`B/bhkWorld.h:42`）。朝前打幾條射線（中央 + 左右斜），命中就把方向往空側偏。對「不太複雜地形」這就夠。

### (b) 地面貼合（避免飄空／掉穿）
- 首選 **`RE::TES::GetLandHeight(const NiPoint3& posIn, float& heightOut)`**（`T/TES.h:74`）——直接拿地形高度設 z。
- 備援：從 actor 上方往下打 havok 射線取命中 z（涵蓋地形外的 static 地板，如橋、岩石）。
- 用模式 1 的 char controller velocity 時，havok 自帶重力處理大部分貼地；`GetLandHeight` 主要防「跨 cell 邊界或 LOD 未載入時掉穿」與合理性檢查。

### (c) 自維護輕量 waypoint graph
- plugin 自存一組路點（玩家足跡或預標 marker），用 A* 在自己的圖上求路，每段做 (a) 直線 steering。對「動態生成區域」很合適：生成時順手鋪幾個 waypoint，完全不碰引擎 navmesh。

**評估**：對「不太複雜地形」**(a)+(b) 已足夠**；(c) 是「障礙稍多需繞」時的低成本升級。三者都不需引擎 pathing，全用 vendored API 寫得出。

---

## 5. 坑與緩解

1. **滑步（最常見）**：只搬膠囊不驅動動畫 → idle 姿勢滑行。緩解：§3 模式 1，速度寫入與 `SpeedSampled` 匹配；轉身用 `InterpAngleTo`。
2. **掉穿地形／飄空**：跨 cell、LOD 未載入、或 `SetPosition` z 設錯。緩解：每幀用 `GetLandHeight` 夾 z 下限；優先用 char controller velocity 讓 havok 自管重力，而非硬寫 z。
3. **Havok 與手寫位置打架**：同時用 `SetPosition`（瞬移）又用 havok velocity 會抽搐/回彈。緩解：**只選一種位移管道**——建議統一走 `SetLinearVelocityImpl`，別混 `SetPosition`。
4. **package stack 搶回控制權**：只要 actor 還有有效 AI package，引擎每次 `EvaluatePackage`（`A/Actor.h:523`）會嘗試重新接管移動。緩解（與 package 分析協調）：用 `EnableAI(false)`（`A/Actor.h:521`）暫停 AI 由你全權驅動，或塞「DoNothing」package 佔位；移動結束再 `EnableAI(true)` + `EvaluatePackage`。`AIProcess` 有 `skippedTimeStampForPathing` 旗標（`A/AIProcess.h`，offset 0x138），印證引擎 pathing 與此流程綁定。
5. **frame-rate 依賴**：`pos += dir * speed` 在不同幀率下速度不一致。緩解：插值用 `MathUtil::Interp::InterpTo(current, target, deltaTime, interpSpeed)`（`src/util.h:332`，已 frame-rate 無關），位移用 `velocity * deltaTime`。
6. **主執行緒**：所有對 actor/havok 的寫入要在主執行緒（`SKSE::GetTaskInterface()->AddTask` 或每幀 hook），否則崩。
7. **狀態旗標**：移動時同步 `ActorState1.walking/running`（`A/ActorState.h:106-107`，唯讀查詢 `IsWalking()`/`IsRunning()`）語意一致，避免其他系統誤判靜止。

---

## 6. 綜合判定

- **最務實方案**：自製 steering（朝目標直線 + havok 射線避障）+ `TES::GetLandHeight`/向下射線貼地 + 每幀寫 `bhkCharacterController::SetLinearVelocityImpl` 推膠囊 + 餵 `SpeedSampled`/`Direction`/(視需要)`bAnimationDriven` 驅動走路動畫，全程 `InterpTo`/`InterpAngleTo` 做平滑與 frame-rate 無關。移動期間 `EnableAI(false)` 拿走 package 控制權。對「不太複雜平地」可達成可信步行位移。
- **不可行**：① 從 SKSE 動態生成 navmesh（precomputed form）；② 用型別化 API 餵自製路徑給引擎 pathing（`BSPathingSystem`/`PathingRequest`/`MovementMessageNewPath` 在 NG **只有 RTTI/VTABLE，無類別宣告**）；③ 靠 `KeepOffsetFromActor`（仍依賴 navmesh，且 NG 未 vendored）。

### 社群先例（已驗證 vs 道聽途說）
- **「actor 需要 navmesh」**——已驗證社群/官方共識（CK wiki、gamesas）：navmesh 缺塊 actor 就到不了；引擎 NPC 移動以 navmesh 求路為前提。本題要繞過的根因。
- **`bAnimationDriven` 切 animation-driven**——gamesas 串明確給出 `SetAnimationVariableBool("bAnimationDriven", true)` 使 actor 由動畫驅動位移；可驗證技術。
- **AMR**——讓引擎正確讀動畫 root-motion 位移、消滑步；佐證 animation-driven 位移可行但有坑（vanilla 在地面時錯把 Y 位移依 pitch 轉到 Z）。
- **True Directional Movement (ersh1, 開源)**——SKSE plugin，全程操作 movement/graph 變數做 360° 移動與朝向；「SKSE 直接驅動 locomotion 圖變數」的最佳開源範本（`github.com/ersh1/TrueDirectionalMovement`），值得抄其讀寫 graph variable 做法。**未逐行驗證其是否用 `SetLinearVelocityImpl`**——待查線索而非結論。
- **`TranslateTo` 搬 actor**——gamesas 實證：對 actor 只保留地面碰撞、會穿過其他物件，滑步嚴重 → 印證 keyframe 不適合當主位移管道。

### 最小可行 PoC（證明 A→B 可走，按風險排序）
1. 取一個 NPC，`EnableAI(false)`，每幀（hook 或 task）：
2. `dir = normalize(B - actor->GetPosition())`；用 `InterpAngleTo` 平滑把朝向轉到 `dir`。
3. `actor->GetCharController()->SetLinearVelocityImpl(hkVector4{dir.x*spd, dir.y*spd, vz, 0})`（vz 交給重力或 0），`spd` 由 `GetWalkSpeed()`（`A/Actor.h:436`）取。
4. 同步 `SetGraphVariableFloat("SpeedSampled", spd)` 與 `SetGraphVariableFloat("Direction", 0)`（正前方）讓走路動畫播出來。
5. 每幀 `TES::GetSingleton()->GetLandHeight(pos, h)` 夾 z 下限防掉穿。
6. 到達 B（距離 < 閾值）→ 速度歸零、`EnableAI(true)` + `EvaluatePackage`。

**最該先解的單一未知數**：第 3+4 步——**寫 char controller 線速度時，graph 餵了 `SpeedSampled`/`Direction` 後，腳步動畫會不會與實際位移同步（不滑步）**。先只在「一塊沒有 navmesh 的平地」上把這個閉環跑通；若滑步無法接受，再決定是否升級到 `bAnimationDriven` + root-motion 走路動畫（模式 2）。避障射線、waypoint graph 都是這閉環成立後的增量。

---

### 引用標頭
`A/Actor.h`、`A/ActorState.h`、`A/AIProcess.h`、`A/ActorMover.h`、`M/MovementControllerNPC.h`、`M/MovementControllerAI.h`、`M/Movement.h`、`I/IMovementMessageInterface.h`、`I/IMovementDirectControl.h`、`I/IAnimationGraphManagerHolder.h`、`B/bhkCharacterController.h`、`B/bhkCharProxyController.h`、`B/bhkWorld.h`、`B/bhkPickData.h`、`B/BGSMovementType.h`、`H/hkpWorldRayCastInput.h`、`H/hkpWorldRayCastOutput.h`、`T/TES.h`、`T/TESObjectREFR.h`、`T/TESObjectCELL.h`、`N/NavMesh.h`、`F/FixedStrings.h`、`Offsets_RTTI.h`、`Offsets_VTABLE.h`，及 repo `src/util.h`（`TranslateTo`/`InterpTo`/`InterpAngleTo`/`GetForwardVector`/`GetNiPoint3`/`NifUtil::Collision`）。

**Sources**
- [GECK Navmesh Creation Workflow](https://geckwiki.com/index.php/Navmesh_Creation_Workflow) / [CreationKit Navmesh](https://ck.uesp.net/wiki/Category:Navmesh)
- [gamesas: TranslateTo collisions](https://www.gamesas.com/getting-collisions-move-with-translateto-t260961.html)
- [gamesas: make Animation Events move an actor (bAnimationDriven)](http://www.gamesas.com/found-out-how-make-animation-events-actually-move-t258307.html)
- [Animation Motion Revolution](https://www.nexusmods.com/skyrimspecialedition/mods/50258)
- [True Directional Movement (source)](https://github.com/ersh1/TrueDirectionalMovement)
