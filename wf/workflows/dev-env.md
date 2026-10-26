# dev-env — 開發環境、指令、外部工具

[WORKFLOWS](../WORKFLOWS.md)｜[INDEX](../INDEX.md)

這台機器上要能開發需要什麼、怎麼裝、跑什麼指令；外部工具設定與 env var 也收在這裡。

**何時用**：fresh clone、換機器、裝不起來、忘了指令、要加一個外部工具或環境變數。
**何時不用**：驗證 / 測試怎麼跑 → [testing](testing.md)；程式碼慣例 → [common/conventions](common/conventions.md)。

## Done when

- 照「流程（fresh clone 後）」走完，`cmake --build build/release-clang-cl-linux`（Linux）或 `cmake --build build/release-msvc`（Windows）回傳 0。
- 下面三張表沒有空欄；要使用者親自做的（帳號、授權、金鑰）在 [WAIT_USER](../../../../WAIT_USER.md)（母 repo）各佔一行。

**完整指令與細節的唯一出處**是根 [BUILD.md](../../BUILD.md)（Windows／PowerShell、dumpbin 檢查、clean rebuild）與 [FOR_AGENT.md](../../FOR_AGENT.md)（Manjaro 交叉編譯→打包→看 log 的 SOP）；本檔只做索引，兩邊有出入以那兩份為準。

## 流程（fresh clone 後）

1. 沒有 git submodule；CommonLibSSE-NG 走 vcpkg port（`vcpkg-configuration.json` 的 registry），不用另外 clone。
2. 裝 vcpkg 並設 `VCPKG_ROOT`；Windows 另裝 VS 2022「Desktop development with C++」，Linux 另裝 clang-cl／lld-link／xwin（見 [FOR_AGENT.md](../../FOR_AGENT.md)）。
3. 視需要設 `SKYRIM_FOLDER` 或 `SKYRIM_MODS_FOLDER`（build 後自動複製 dll；不設就留在 build 目錄）。
4. 跑一次 configure＋build（下表）確認裝好了。

## 指令表

| 做什麼 | 指令 | 備註 |
|--------|------|------|
| configure（Linux）| `cmake --preset build-release-clang-cl-linux` | `vcpkg.json`／triplet 改過要先砍 `build/` 重來 |
| build（Linux）| `cmake --build build/release-clang-cl-linux -j$(nproc)` | 只做編譯驗證；出貨 dll 走 Windows CI |
| configure＋build（Windows）| `cmake --preset build-release-msvc; if ($?) { cmake --build build/release-msvc }` | PowerShell，不用 `&&`；Debug 換 `debug-msvc` |
| 打包 MO2 zip | `./scripts/pack_env.sh --config release-clang-cl-linux`（Linux）／`scripts/pack.ps1`（Windows）| 產物在 `dist/` |
| lint / format | 無設定 | — |

驗證與測試指令不列這裡——連同「誰跑」一起在 [testing](testing.md)。

## 跨機 / 離線差異

| 環境 | 能跑 | 跑不了的 → 怎麼辦 |
|------|------|------------------|
| Windows（VS 2022＋MSVC）／GitHub Actions `windows-latest` | 全部：build、`dumpbin` 檢查、打包、出貨 dll（CI 見 [CI.md](../../CI.md)）| — |
| Manjaro 主力機（clang-cl＋xwin 交叉編譯）| 編譯驗證、`pack_env.sh` 打包、`scripts/*.sh` 契約測試、Proton 下實機看 log | 出貨 dll 交 Windows CI；實機驗收記 [WAIT_USER](../../../../WAIT_USER.md) |

## 外部工具與 env var

| 名稱 | 用途 | 怎麼取得 / 設定 |
|------|------|----------------|
| vcpkg（`VCPKG_ROOT`）| 依賴管理（CommonLibSSE-NG、simpleini、nlohmann-json…）| clone＋bootstrap，`VCPKG_ROOT` 指向它 |
| clang-cl／lld-link／xwin | Linux 交叉編譯到 Windows x64 | 見 [FOR_AGENT.md](../../FOR_AGENT.md)、`cmake/toolchain-clang-cl-xwin.cmake` |
| `SKYRIM_FOLDER` | build 後把 dll 複製到 `<值>/Data/SKSE/Plugins/` | 可選，本機路徑不進 repo |
| `SKYRIM_MODS_FOLDER` | build 後複製到 MO2／Vortex 的 `<值>/<ProjectName> <BuildType>/SKSE/Plugins/` | 可選，本機路徑不進 repo |

需要帳號、付費、授權才能取得的：守鐵律 2（授權來源），並在 [WAIT_USER](../../../../WAIT_USER.md) 記一行。

## 交接

- 環境就緒要開工 → [feature-dev](feature-dev/README.md)；先確認驗證跑得動 → [testing](testing.md)。
- 同一個裝機坑第二次撞到 → [common/gotchas](common/gotchas.md)。
