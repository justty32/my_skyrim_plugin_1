# CI（GitHub Actions）

Windows runner 自動 build + 打包 plugin。Workflow 在 `.github/workflows/build.yml`。

## 什麼時候跑

- Push 到 `main`
- 任何 PR（含 fork PR）
- 手動：GitHub UI 的 `Actions` tab → `Build` → `Run workflow`

## 每次跑做什麼

1. Checkout
2. 跑 `scripts/test_packaging.ps1` 驗證離線打包契約
3. 設 MSVC 開發環境（`ilammy/msvc-dev-cmd@v1`，效果等同 Developer PowerShell for VS 2022；參見 `PITFALLS.md` 第 4 條的背景）
4. 安裝 Ninja
5. 把 `VCPKG_INSTALLATION_ROOT`（windows-latest 預裝 vcpkg 在 `C:\vcpkg`，環境變數這樣叫）alias 成 `VCPKG_ROOT`，`CMakePresets.json` 才讀得到
6. 還原 vcpkg binary cache（見下節）
7. `cmake --preset build-release-msvc` → `cmake --build build/release-msvc`
8. **驗證靜態 CRT**：跑 `dumpbin /dependents`，若 DLL 依賴 `MSVCP*.dll` / `VCRUNTIME*.dll` 就 **fail 整個 workflow**（靜/動 CRT 搞混了）
9. 跑 `scripts/pack.ps1` 打包 MO2 zip
10. 上傳 artifact：
   - `DaylightDungeon-dll` — 單一 `.dll`
   - `DaylightDungeon-mo2-zip` — MO2 可直接安裝的 `.zip`

## Cache

首次 build 要從頭編所有 vcpkg deps（`commonlibsse-ng-fork` 一個人就要 10–20 分鐘）。之後靠 vcpkg binary cache：

```
VCPKG_BINARY_SOURCES=clear;files,<cache-dir>,readwrite
```

vcpkg 會把每個 port 編好的 `.lib` + headers 存到 cache dir。`actions/cache@v4` 的 key 綁下列三個檔的 hash：

- `vcpkg.json` — 相依清單
- `vcpkg-configuration.json` — registry baseline
- `cmake/x64-windows-skse.cmake` — triplet（改靜/動 CRT 會失效）

改這三個檔其中一個 → cache key 變 → 整包 deps 重編。改其他東西（plugin code、CMakeLists、preset 的非 triplet 部份）→ 命中既有 cache → build 時間從 20 分鐘降到 2–3 分鐘。

`restore-keys` 有 fallback：精確 key 沒命中時，載入最近的前一個 cache，vcpkg 只會重編真的變動的 port。

## 取 artifact

1. 打開該次 Actions run（`https://github.com/<你>/<repo>/actions`）
2. 拉到頁面底部「Artifacts」區塊下載

Artifact 保留 90 天（GitHub 預設），之後要自己留一份或重跑 workflow。

## 費用

**公開 repo = 完全免費無限分鐘**，Linux / Windows / macOS runner 都無配額。私有 repo 才有每月 2000 Linux-min / 1000 Windows-min 的免費額度（Windows 計 2 倍）。

## 啟用前需要做的事

1. **Repo 要是你自己的**。目前 `origin` 指向 `Monitor221hz/CommonLibSSE-NG-Template-Plugin`（上游 template，不是你的）。你沒有那邊的 write 權限，push 不上去 → CI 不會跑。先換 remote：

   ```powershell
   # 方案 A：手動在 github.com 建好 repo 後
   git remote set-url origin https://github.com/<你的帳號>/<repo-name>.git
   git push -u origin main

   # 方案 B：用 gh CLI
   gh auth login          # 只需做一次，會開瀏覽器
   gh repo create --source . --public --push
   ```

2. 公開 repo 的 Actions 功能**預設啟用**，不用動設定。
3. 第一次 push 之後看 Actions tab，若 fail 回報 log。

## Plugin 名稱與發佈契約

Workflow 在 build 後從 `CMakeCache.txt` 讀取 `CMAKE_PROJECT_NAME`，先確認對應 DLL 確實存在，再把精確路徑交給 `dumpbin` 和 artifact upload。名稱不再硬編碼於 workflow。

設定檔子目錄由 `CMakeLists.txt` 的 cache 變數 `PLUGIN_CONFIG_FOLDER` 定義；CMake post-build、`pack.ps1` 與 `pack.sh` 共用此值。若改 plugin 名稱而設定檔目錄也應跟著改，只需同時更新 `project(...)` 與 `PLUGIN_CONFIG_FOLDER`。可用 `scripts\test_packaging.ps1` 離線驗證這條契約。

## 未來可加

- **Release workflow**：`git tag v0.0.1 && git push --tags` → 自動把 zip 附到 GitHub Releases
- **Debug build**：matrix 加 `debug-msvc`，給沒有 Windows 機但想偶爾取 debug DLL 的情境（你目前 debug 走重開機，應該用不到）
- **PR check**：clang-format、cppcheck 之類 lint

要做告訴我。
