# conventions — 程式碼慣例（碰原始碼的工作流共用）

[common/README](README.md)｜[INDEX](../../INDEX.md)

碰原始碼的工作流（feature-dev / refactor / investigation）共用這套規矩：**寫碼時要遵守什麼**。哪個檔負責什麼領域、測試在哪 → [code-map](code-map.md)；真相層優先序也在 [code-map](code-map.md)。結構整理原則 → [STRUCTURE](../../STRUCTURE.md)。

來源：根 [AGENTS.md](../../../AGENTS.md)（Build、Adding source files、Code layout）、[FOR_AGENT.md](../../../FOR_AGENT.md) §2、[PITFALLS.md](../../../PITFALLS.md)；細節與緣由以那幾份為準。

## 慣例

| 項目 | 規矩（怎麼檢查）|
|------|------|
| 檔案註冊 | 不用 CMake glob；新 `.cpp` 進 `cmake/sourcelist.cmake`、新 `.h` 進 `cmake/headerlist.cmake`（漏了就不會編）|
| 插件資訊 | 原始碼裡不寫 `SKSEPluginInfo(...)`，由 `CMakeLists.txt` 的 `add_commonlibsse_plugin` 產生（重複＝LNK2005）；`grep -rn SKSEPluginInfo src` 應為空 |
| CRT | 靜態 CRT：`CMakePresets.json` 的 `CMAKE_MSVC_RUNTIME_LIBRARY` 與 `cmake/x64-windows-skse.cmake` 的 `VCPKG_CRT_LINKAGE` 要同改，改完砍 `build/`（否則 LNK2038）|
| helper | 新工具函式優先擴充 `src/util.h` 既有命名空間，不另立平行 helper |
| log | Proton 下不 attach debugger，關鍵步驟、指標檢查、Form 建立成敗都要 `SKSE::log::*`；`hook.cpp` 不 include `log.h` |
| 引擎結論 | 引擎層的說法要對 CommonLibSSE-NG `RE/` 標頭查證；設計新功能前先讀 `research/`（已有死路紀錄）|
| 註解語言 | `src/` 程式碼註解英文 |
