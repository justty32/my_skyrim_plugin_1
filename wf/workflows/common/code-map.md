# code-map — 程式碼導航 index（哪個檔負責什麼）

[common/README](README.md)｜[INDEX](../../INDEX.md)

碰原始碼前先查這張表，只讀相關領域列出的檔；動完再照維護鏈把表更新回去。寫碼慣例本身在 [conventions](conventions.md)。

## 領域表

本專案的 code map 本體是下面的「`src/` 逐檔職責」（2026-10-26 自根 [AGENTS.md](../../../AGENTS.md)「Code layout」段移入，原文照搬）；非建置目錄（`research/`、`vendor/`、`archive/`）見根 AGENTS.md「Non-build directories」段。下文的「本表」都指這一段。

### `src/` 逐檔職責

- `src/plugin.cpp` — entry point (`SKSEPluginLoad`). Calls `SKSE::Init`, `SetupLog()`, then registers `MessageHandler` on the SKSE messaging interface. `kDataLoaded` runs `OnDataLoaded()` (`NpcGenerator::InitializeMagic`, `FollowLight::Initialize`, `AmbientBoost::Initialize`, `Hooks::Install`); `kNewGame` / successful `kPostLoadGame` call `NpcGenerator::GiveSpellsToPlayer`; `kPreLoadGame` calls `AmbientBoost::RestoreAll` so a session's cell-ambient edits don't leak into the loaded save.
- `src/NpcGenerator.{h,cpp}` — NPC/spell logic (magic setup at data-load, grants spells to the player on new game / load).
- `src/FollowLight.{h,cpp}` — a runtime fill light attached to the player (hotkey, default `L`, cycles brightness tiers); configured by `config/FollowLight.ini`. Needs a per-frame driver — see `hook.cpp`.
- `src/AmbientBoost.{h,cpp}` — hotkey (default `K`) cycles levels that raise the current interior cell's ambient / directional / DALC lighting (takes effect on cell re-entry); `RestoreAll()` undoes this session's edits. No per-frame hook needed.
- `src/core/DeterministicRandom.{h,cpp}` — cross-language deterministic random primitive for `QUEST_ENGINE_SPEC.md` appendix B; zero game/runtime dependencies. **Not** in `sourcelist.cmake` — add it there if a runtime module starts using it.
- `src/hook.{h,cpp}` — `Hooks::Install()` writes a vtable hook on `PlayerCharacter::Update` (Actor vfunc `0xAD`) that calls the original then `FollowLight::Update()`. It's a `write_vfunc`, not a code trampoline, so no `SKSE::AllocTrampoline` is needed. `hook.cpp` must not include `log.h` (that header defines `SetupLog()`; `SKSE::log` comes in via the PCH).
- `src/log.h` — `SetupLog()` creates an spdlog basic file sink at `SKSE::log::log_directory() / "<PluginName>.log"` with trace-level logging and flush-on-trace. Log via `SKSE::log::info(...)` etc.
- `src/settings.h` — still an empty stub; `simpleini` and `nlohmann-json` are already vcpkg dependencies, so config parsing is wired up headers-wise (SimpleIni include dir is added in `CMakeLists.txt`).
- `src/util.h` — large header of static helper structs inside namespaces. When adding utilities, prefer extending these instead of inventing parallel helpers:
  - `PointerUtil::adjust_pointer` — cv-preserving pointer arithmetic.
  - `SystemUtil::File::GetConfigs` — enumerate config files by suffix/extension.
  - `KeyUtil` — `MACRO_LIMITS` / `KBM_OFFSETS` / `GAMEPAD_OFFSETS` enums and `Interpreter::GamepadMaskToKeycode` mapping game gamepad bitmasks to the 282-key macro space.
  - `Util::String` — `Split`, `Join`, `iContains`, `iEquals`, `ToLower`, `ToUpper`, `ToFloatVector`.
  - `MathUtil` — `Clamp`, `ApproximatelyEqual`, `GetNiPoint3(hkVector4)`, and `MathUtil::Angle` (degree/radian conversion, angle normalization, quaternion↔matrix, `GetForwardVector`). Predefined `PI`, `TWO_PI`, `PI2`/`PI3`/`PI4`/`PI8`, `TWOTHIRDS_PI` macros.
  - `MathUtil::Interp::InterpTo`, `ObjectUtil::Transform::InterpAngleTo` — frame-rate-independent interpolation.
  - `ObjectUtil::Transform::TranslateTo` — wraps `RELOCATION_ID(55706, 56237)`.
  - `AnimUtil::Idle::Play` — wraps `RELOCATION_ID(38290, 39256)` to play idle animations on an actor.
  - `FormUtil::Parse` — resolve `TESForm*` / `FormID` from `"formid~modname"` config strings (delimiter defaults to `~`).
  - `FormUtil::Quest::FindAliasByName` — locks `aliasAccessLock`, iterates aliases.
  - `NifUtil::Node` / `Armature` / `Collision` — clone NiAVObject (`RELOCATION_ID(68835, 70187)`), find/attach bones by name on an actor, and toggle/remove havok collision via `BSVisit::TraverseScenegraphCollision` with `CFilter::Flag::kNoCollision`.
  - `util.h` does `using namespace RE;` at the top, so `RE::` prefixes are optional inside these namespaces but still encouraged for clarity elsewhere.

### 總表

| 領域 | 檔案 | 職責 | 測試在哪 |
|------|------|------|---------|
| 全部 | `src/**`（註冊清單 `cmake/sourcelist.cmake`、`cmake/headerlist.cmake`）| 見上方「`src/` 逐檔職責」| 無 C++ 測試套件；契約測試在 `scripts/test_*.{sh,ps1}`、`tools/quest_prf_test.cpp`（見 [testing](../testing.md)）|

## 真相層優先序

各專案可以改自己的優先序，但必須明確。預設：

```text
code/tests > schema/examples/fixtures > code map > docs > generated
```

- 上層與下層衝突時，**以上層為準並修正下層**。
- generated（產生出來的檔、html）永遠不是唯一真相。
- 原始來源與摘要衝突時，以**原始來源**為準。
- code map 是**導航不是規格**；行為以 code/tests 為準。

## 維護鏈：程式碼 > code map > 文檔

**優先級**（衝突或時間不夠時，依序保持一致）：程式碼 > code map > 文檔。
**code map 與程式碼衝突時以程式碼為準，立刻改 code map。**

1. **修改前**：先讀本表找到相關領域，只讀清單裡的檔——不讀無關領域的檔。
2. **修改後**：新增／刪除了原始碼檔案，或某檔職責顯著改變，必須同步更新本表。
3. 原始碼裡**不加**「對應 code map」的註釋（維護成本過高）；反向查找直接 grep 本檔。
4. 迭代期間本表可暫時落後，**commit 前必須對齊**（見 [feature-dev](../feature-dev/README.md)）。
