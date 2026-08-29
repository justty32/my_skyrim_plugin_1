# 存檔清理：風險與查證索引

[返回總索引](kpresavegame_dynamic_cleanup.md)

## 8. 開放風險與 TODO 清單

- ⚠️ **「延後一週期」限制（方案 A/B 共有）**：當次 `.ess` 在 cleanup task 跑之前就寫好了，
  所以**第一個含召喚物的存檔仍可能 crash 於冷重啟**；要乾淨需「存過 ≥1 次後續檔」。
  若需要當次根治 → 方案 C。請在實作 commit message / progress.md 明確標註此特性，避免誤判為失敗。
- ⚠️ **`SetDelete` 後同 frame 的序列化交互**：方案 C 若 hook `Save` 入口、在序列化前同步刪 actor，
  需驗證引擎不會在序列化一個剛 `SetDelete` 的 ref 時崩潰。**未驗證**，列 TODO。
- ⚠️ **執行緒**：`kSaveGame` 訊息 / co-save `OnSave` 皆不保證主執行緒——本設計一律經 `AddTask`
  丟主執行緒再做 RE:: 操作。實作時**勿**在 callback 直接 `Disable()/SetDelete()`。
- ⚠️ **lambda 捕捉**：`AddTask` 的 lambda 只能捕 FormID（by value），task 內 `LookupByID` 重解析；
  **絕不**捕 `TESObjectREFR*` / handle 跨 frame（`ProcgenItem.cpp:505-510` 鐵律）。
- 📌 **TODO（方案 C，勿憑空填）**：找 `BGSSaveLoadManager::Save`（`BGSSaveLoadManager.h:87`）或
  `Save_Impl`（`:190`）的 SE/AE RELOCATION_ID（兩 runtime 都要）；確認 hook 觸發執行緒；
  `src/hook.{h,cpp}` 目前是空殼，須先建 trampoline。**沒有 ID 前不要寫 C 方案。**
- 📌 **ProcgenItem 是否要納入清理**：物品族（WEAP/ARMO/MISC）會被引擎序列化但重載成空殼，
  非 crash 主因（`ProcgenItem.h:27-46`）；納入清理是「為一致 / 防累積」，非根治 crash 必需。可後置。
- 📌 **NpcGenerator registry**：若採 Step 2，注意 `SpawnNpc` 目前無 mutex、無追蹤；新增的
  `g_spawned` 只在主執行緒（spell cast sink / cleanup task）被碰，理論上不需鎖，但與其他模組一致起見
  建議仍加 `std::mutex`。

---

## 9. 引用到的真實標頭與程式碼位置（供實作者核對）

- `CommonLibSSE-NG/include/SKSE/Interfaces.h`：`MessagingInterface` 列舉 `280-293`（**無 kPreSaveGame，僅 kSaveGame=4**）、
  `Message`/`EventCallback` `265-273`、`RegisterListener` `310-311`；`SerializationInterface`
  `81-99`（`SetSaveCallback`/`SetUniqueID`/`OpenRecord`/`WriteRecordData`/`ResolveFormID`）；
  `TaskInterface::AddTask` `196`。
- `CommonLibSSE-NG/include/RE/T/TESObjectREFR.h`：`SetDelete` `234`（vfunc 23）、`Disable` `337`
  （`SKYRIM_REL_VR_VIRTUAL`，vfunc 89）、`IsDisabled` `442`。
- `CommonLibSSE-NG/include/RE/B/BGSSaveLoadManager.h`：`Save(const char*)` `87`、`Save_Impl` `190`、
  非同步存檔 `Thread` `54-72`。
- `src/plugin.cpp`：`MessageHandler` `41-97`、`SKSEPluginLoad` 內 co-save 註冊 `110-129`。
- `src/skyrim/CoSave.{h,cpp}`：中央分派器；`OnSave` fan-out `CoSave.cpp:22-27`、`Register`/`SetSaveCallback`
  `CoSave.cpp:80-94`、執行緒規則 `CoSave.h:26-29`。
- `src/skyrim/procgen/ProcgenNpc.cpp`：`Registry()` `58-61`、`TrackedNpc.ref/priorRefFormID` `42/52`、
  同 session 重鑄拆除 `271-277`、re-adopt 分支 `467-489`、`OnSave` 寫 recipe `301-336`、
  `OnRevert` `405-412`、`RebuildStaged` `427-536`。
- `src/skyrim/procgen/Procgen.cpp`：`Rooms()` `54-57`、handle 向量 `44-46`、**`DropRoom` `256-270`**、
  `ClearGenerated` `767-786`、`OnSave` `822-847`。
- `src/skyrim/procgen/ProcgenItem.cpp`：`Registry()`/`TrackedItem.live` `78-91`、
  **`StripPriorInstances` `382-436`**、`AddObjectToContainer` `624`、`RebuildStaged` `729-808`、
  lambda/FormID 鐵律 `505-510`。
- `src/NpcGenerator.cpp`：`SpawnNpc`（無追蹤）`60-99`、`SpellCastHandler` `212-256`。
- `research/PROCGEN_NPC_FORMS.md`：§3.2 序列化型別清單、§5「時機」co-save round-trip、
  `IsDynamicForm()`=`>=0xFF000000` `325`。
- `progress.md:31`（kPostLoadGame 刪 codec actor 致命 → re-adopt 修法）、`:40`（跨 session crash
  未解、`C++: Spawn NPC` 主嫌、需 `kPreSaveGame` 鉤子根治）、`:46`（下一步列為 `kPreSaveGame` 清理）。
