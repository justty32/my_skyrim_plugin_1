# Agent Workflow Guide (Manjaro Linux Development)

This document defines the standard operating procedures for an AI agent to build, package, and verify the SKSE plugin in this environment.

## Environment Context
- **Host OS:** Manjaro Linux
- **Target OS:** Windows x64 (Cross-compiled)
- **Compiler:** `clang-cl` via `xwin` toolchain
- **Game Location:** Steam/Proton (Prefix 489830)

## 1. Build Process
To compile the Windows DLL on Linux, use the `release-clang-cl-linux` preset:

```bash
cmake --build build/release-clang-cl-linux -j$(nproc)
```

**Troubleshooting:** If the build command fails, it may be due to a stale cache or configuration mismatch. Re-run the configure preset before attempting to build again:
```bash
cmake --preset build-release-clang-cl-linux
```

## 2. Packaging Process
After a successful build, use the environment-aware packaging script. This will generate a ZIP file (usually in the `dist/` folder or your mod manager's directory if configured):

```bash
./scripts/pack_env.sh --config release-clang-cl-linux
```

## 3. Deployment & Verification
The plugin must be installed via Mod Organizer 2 (MO2). Once the game is running via Proton, verify the status using the log file.

### Log File Path
The SKSE log is located inside the Proton prefix:
```bash
/home/lorkhan/.local/share/Steam/steamapps/compatdata/489830/pfx/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/TemplatePlugin.log
```

### Verification Command
Use this to check the last modification time and recent logs:
```bash
ls -l "/home/lorkhan/.local/share/Steam/steamapps/compatdata/489830/pfx/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/TemplatePlugin.log"
tail -n 50 "/home/lorkhan/.local/share/Steam/steamapps/compatdata/489830/pfx/drive_c/users/steamuser/Documents/My Games/Skyrim Special Edition/SKSE/TemplatePlugin.log"
```

## Summary of Success Criteria
1. `cmake --build` completes without errors.
2. `pack_env.sh` generates a new `.zip` file in the output directory.
3. The log file timestamp updates after the game starts.
4. The logs do not contain `[error]` or `[critical]` tags.
