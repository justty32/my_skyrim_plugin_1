# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

SKSE plugin template for Skyrim SE/AE/GOG/VR, built on [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) (via the `commonlibsse-ng-fork` vcpkg port from the `Monitor221hz/modding-vcpkg-ports` registry — see `vcpkg-configuration.json`). Hook IDs / address-library offsets for each game version still have to be found manually.

The CMake project name (`TemplatePlugin` in `CMakeLists.txt`) becomes the `.dll` filename. Rename it when using this template for a real plugin, and also update the hardcoded `CONFIG_FOLDER` string (`Template_Plugin`) around `CMakeLists.txt:60` — that's the subfolder name under `SKSE/Plugins/` that the `config/` tree gets copied to on post-build.

## Build

Full command-line workflow (PowerShell + bash, dumpbin verification, clean-rebuild notes) lives in `BUILD.md`. Past-mistakes log (LNK2005 from duplicate `SKSEPluginInfo`, LNK2038 from mismatched CRT, `.exp`/`.lib` without `.dll` meaning link silently failed, C1083 `<tuple>` from missing VS dev env) lives in `PITFALLS.md` — consult it before diagnosing a link-time failure. CI (GitHub Actions `windows-latest`, vcpkg binary cache, auto-upload `.dll` + MO2 zip artifacts) is documented in `CI.md`; workflow lives in `.github/workflows/build.yml`. If the user is unfamiliar with GitHub Actions concepts (workflow / job / step / runner / action / artifact / cache / secret), point them at `GITHUB_ACTIONS.md` — it's a Chinese-language primer tied back to this repo's workflow. Summary:

Requires MSVC (VS 2022), Ninja, and vcpkg with `VCPKG_ROOT` set. Presets live in `CMakePresets.json`.

PowerShell (this is a Windows/pwsh workflow — do not use bash chaining like `&&`):

```powershell
cmake --preset build-debug-msvc;   if ($?) { cmake --build build/debug-msvc }
cmake --preset build-release-msvc; if ($?) { cmake --build build/release-msvc }
```

- Configure presets: `build-debug-msvc`, `build-release-msvc`. Build presets of the same name minus the `build-` prefix (`debug-msvc`, `release-msvc`) exist for `cmake --build --preset ...`.
- Binary dirs: `build/debug-msvc`, `build/release-msvc`.
- C++23, MSVC with `/permissive- /Zc:preprocessor /EHsc /MP`, **static CRT** (`/MT` release, `/MTd` debug — `CMAKE_MSVC_RUNTIME_LIBRARY` in `CMakePresets.json`, `VCPKG_CRT_LINKAGE static` in `cmake/x64-windows-skse.cmake`). Everything links statically into the .dll so it runs on Manjaro/Proton without needing vcredist installed in the prefix. If you change either of those knobs you must change both to match, and wipe `build/` so vcpkg rebuilds all deps against the new CRT — otherwise the final link dies with LNK2038 runtime library mismatch.
- Triplet `x64-windows-skse` (overlay in `cmake/x64-windows-skse.cmake`); library linkage is static for non-SKSE ports and dynamic for ports matching `fully-dynamic-game-engine|skse|qt*`.
- `src/PCH.h` is the required precompiled header — it includes `RE/Skyrim.h` and `SKSE/SKSE.h` and enables `using namespace std::literals`.
- No test suite is configured (`testPresets` in `CMakePresets.json` is empty).

### Install-on-build

Post-build steps in `CMakeLists.txt` copy the `.dll` (and `.pdb` in Debug) into `SKSE/Plugins/` under whichever output root is defined:

- `SKYRIM_FOLDER` env var → `<SKYRIM_FOLDER>/Data/SKSE/Plugins/`
- `SKYRIM_MODS_FOLDER` env var (MO2/Vortex) → `<SKYRIM_MODS_FOLDER>/<ProjectName> <BuildType>/SKSE/Plugins/`

If a `config/` folder exists at the repo root, its contents are also copied to `SKSE/Plugins/<CONFIG_FOLDER>/` (the `CONFIG_FOLDER` name is set in `CMakeLists.txt`, currently `Template_Plugin`). If neither env var is set, the DLL just stays in the build dir.

## Adding source files

`CMakeLists.txt` does not glob. New `.cpp` files must be registered in `cmake/sourcelist.cmake` and new headers in `cmake/headerlist.cmake`, or they won't be built.

## Code layout

- `src/plugin.cpp` — entry point (`SKSEPluginLoad`). Calls `SKSE::Init`, `SetupLog()`, then registers `MessageHandler` on the SKSE messaging interface. The handler has stub `switch` cases for `kDataLoaded`, `kPostLoad`, `kPreLoadGame`, `kPostLoadGame`, `kNewGame` — wire up initialization there.
- `src/log.h` — `SetupLog()` creates an spdlog basic file sink at `SKSE::log::log_directory() / "<PluginName>.log"` with trace-level logging and flush-on-trace. Log via `SKSE::log::info(...)` etc.
- `src/hook.{h,cpp}` — empty stubs for trampoline/hook code.
- `src/settings.h` — empty stub; `simpleini` and `nlohmann-json` are already vcpkg dependencies, so config parsing is wired up headers-wise (SimpleIni include dir is added in `CMakeLists.txt`).
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

## Debugging

Attach a debugger only with a legal Skyrim copy whose `.exe` has been stripped with Steamless. MO2 users need `-forcesteamloader` as an SKSE argument so plugins load with the stub removed. A minimal VS Code `launch.json` (`cppvsdbg`, `request: attach`) example is in `README.md`.
