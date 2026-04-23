# TemplatePlugin

An SKSE plugin for Skyrim SE / AE (incl. 1.6.1170) / GOG / VR, built on [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG). Forked from [Monitor221hz/CommonLibSSE-NG-Template-Plugin](https://github.com/Monitor221hz/CommonLibSSE-NG-Template-Plugin) with extra build infrastructure added on top — see below.

> Rename `TemplatePlugin` in `CMakeLists.txt` (`project(...)`) and the `CONFIG_FOLDER` string near line 60 when using this as a starting point for a real plugin; also update the two hardcoded paths in `.github/workflows/build.yml` and `$ConfigFolderName` in `scripts/pack.ps1`.

## What this adds on top of the upstream template

- **Static CRT build** — produced `.dll` has no dependency on `MSVCP140.dll` / `VCRUNTIME140.dll`; runs on Proton / Wine prefixes without `vcredist`
- **MO2 packaging script** — `scripts/pack.ps1` assembles a `Data/SKSE/Plugins/...` zip ready to install via MO2's archive importer
- **CI (GitHub Actions)** — every push builds on `windows-latest`, caches vcpkg port binaries, verifies the static-CRT build via `dumpbin`, and uploads the `.dll` + MO2 zip as artifacts. Free on public repos.
- **Lifecycle logging** — `MessageHandler` in `src/plugin.cpp` logs every SKSE message type, including player name on save load
- **Documentation**:
  - [`BUILD.md`](BUILD.md) — PowerShell / bash build commands, clean-rebuild rules, `dumpbin` verification
  - [`PITFALLS.md`](PITFALLS.md) — traps this project has hit (LNK2005 from duplicate `SKSEPluginInfo`, LNK2038 from mismatched CRT, silent link failures, `<tuple>` not found, ...)
  - [`CI.md`](CI.md) — how the GitHub Actions workflow is put together; cache key strategy; what to change when renaming
  - [`GITHUB_ACTIONS.md`](GITHUB_ACTIONS.md) — primer on Actions concepts (workflow / job / step / runner / action / artifact / cache), 中文

## Requirements

- Windows 10/11 with Visual Studio 2022 (Community edition works) — "Desktop development with C++" workload
- [`vcpkg`](https://github.com/microsoft/vcpkg) bootstrapped, with `VCPKG_ROOT` environment variable pointing at the clone
- (Optional) `SKYRIM_FOLDER` or `SKYRIM_MODS_FOLDER` env var if you want the post-build step to drop the `.dll` directly into the game folder / a mod manager's mods folder

## Quick start

```powershell
# Configure + build (Release)
cmake --preset build-release-msvc
cmake --build build/release-msvc

# Package into MO2-installable zip
scripts\pack.ps1
```

Full build instructions (PowerShell / bash / VS Code) in [`BUILD.md`](BUILD.md).

## Debugging

A debugger can only attach to Skyrim.exe that's been stripped of its Steam DRM using Steamless. MO2 users need `-forcesteamloader` in SKSE arguments. Minimal VS Code `launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "C++ Debugger",
            "type": "cppvsdbg",
            "request": "attach"
        }
    ]
}
```

For runtime on Proton (Skyrim under Linux / Steam Deck), native debugger attach isn't practical; read the plugin log instead:

```
<prefix>/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/<PluginName>.log
```

(Where `<prefix>` is typically `~/.steam/steam/steamapps/compatdata/489830/pfx/`.)

## Acknowledgments

- [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) by CharmedBaryon — the Skyrim RE library this whole ecosystem runs on
- [CommonLibSSE-NG-Template-Plugin](https://github.com/Monitor221hz/CommonLibSSE-NG-Template-Plugin) by Monitor221hz — the starting-point template this project forked from
- [`commonlibsse-ng-fork`](https://github.com/Monitor221hz/modding-vcpkg-ports) vcpkg port, also by Monitor221hz
