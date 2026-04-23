# GitHub Actions 入門

GitHub Actions 的基本概念與語法，以便看懂 `.github/workflows/build.yml`。本專案具體如何使用（cache 策略、artifact、費用、重命名要改哪兩處）看 `CI.md`。

## 是什麼

GitHub 內建的 CI/CD 服務。把 YAML 檔放到 `.github/workflows/` 下，它監聽 repo 的事件（push、PR、tag、定時、手動觸發...），事件發生時在 GitHub 提供的 VM（"runner"）上跑你指定的 step。

「在 VM 上能做的事都能做」：編譯、測試、部署、自動發 release、定時 job、lint、發通知到 Slack / Discord、...

## 核心概念

| 詞 | 解釋 |
| --- | --- |
| **Workflow** | 一個 YAML 檔 = 一個 pipeline。存在 `.github/workflows/*.yml` |
| **Event / Trigger** | 何時跑，寫在 `on:` 欄位。常見 `push`、`pull_request`、`schedule`、`workflow_dispatch`（手動）、`release` |
| **Job** | Workflow 裡的執行單元。在**單一 runner** 上循序跑 step。多個 job 預設**平行** |
| **Step** | Job 內的循序動作。任一 step 失敗整個 job 就掛（除非加 `continue-on-error: true`） |
| **Runner** | 跑 job 的 VM。GitHub 託管：`ubuntu-latest` / `windows-latest` / `macos-latest`；也能 self-host 自己的機器 |
| **Action** | 可重用單元。Step 可以是 `uses: author/name@version`（marketplace action）或 `run: shell 指令` |
| **Artifact** | Job 產出的檔案，打包上傳，UI 可下載，預設保留 90 天 |
| **Cache** | 跨 run 持久化的檔案（node_modules、vcpkg_installed 之類）。由 key 索引，key 命中則還原 |
| **Secret** | 在 repo 設定加密保存的值，workflow 中用 `${{ secrets.NAME }}` 引用。**Fork 的 PR 預設拿不到 secret**（安全） |
| **`GITHUB_TOKEN`** | 每次 run 自動產生的短命 token，呼叫 GitHub API 用（發 release 之類）。預設權限有限，需要寫入時要顯式加 `permissions:` 欄位 |

## 最小範例

```yaml
name: Hello

on: [push]

jobs:
  hello:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: echo "Hello from $GITHUB_REPOSITORY"
```

Push 這個檔上 GitHub → Actions tab 會看到 "Hello" workflow 跑起來，輸出這行訊息。

## 觸發方式（`on:`）

```yaml
# 單一事件
on: push

# 多個事件
on: [push, pull_request]

# 限定分支
on:
  push:
    branches: [main]
  pull_request:        # 所有 PR

# 定時（cron，UTC 時區）
on:
  schedule:
    - cron: '0 3 * * *'   # 每天 UTC 03:00

# 只在標籤 push 跑（用於發 release）
on:
  push:
    tags: ['v*']

# 手動觸發按鈕（Actions tab 會多一顆 Run workflow）
on:
  workflow_dispatch:
    inputs:               # 選填：UI 會幫你產一張表單
      target:
        required: true
        default: release
```

本 repo 的 `build.yml` 是組合：`push to main + PR + 手動`。

## Marketplace Action

`uses: author/action-name@version` 就是引用別人寫好的 step 邏輯，省得每次都重刻 checkout、cache、upload 這類共通事。常見：

- `actions/checkout@v4` — 把 repo clone 到 runner
- `actions/cache@v4` — 存取 cache
- `actions/upload-artifact@v4` — 上傳 artifact
- `ilammy/msvc-dev-cmd@v1` — 設 MSVC 環境變數

**版本 pin 策略**：

- **日常**：pin 到 major（`@v4`）。作者承諾同 major 不出 breaking change，安全修補會自動跟上
- **高安全性**：pin 到 commit SHA（`@abc123def456...`）。完全鎖死，升級要手動改，避免供應鏈攻擊
- **避免**：`@main` / `@master`。作者一 push 你的 CI 行為就可能變，且有被攻擊者 compromise 的 blast radius

找東西先去 https://github.com/marketplace?type=actions 搜。

## 逐段看本 repo 的 `build.yml`

```yaml
name: Build                         # 顯示在 Actions tab 的名字

on:
  push:
    branches: [main]
  pull_request:
  workflow_dispatch:

jobs:
  build:                            # job id，自己命名，可以有多個 job
    runs-on: windows-latest         # 選 Windows runner（因為要 MSVC）

    env:                            # job 層級的環境變數
      VCPKG_BINARY_CACHE_DIR: ${{ github.workspace }}/.vcpkg-bincache

    steps:
      - uses: actions/checkout@v4   # 把 repo clone 到 runner

      - name: Set up MSVC dev environment
        uses: ilammy/msvc-dev-cmd@v1
        with:                       # 傳參數給 action
          arch: x64

      - name: Align VCPKG_ROOT
        run: |                      # 多行指令；Windows 預設 shell 是 pwsh
          echo "VCPKG_ROOT=$env:VCPKG_INSTALLATION_ROOT" >> $env:GITHUB_ENV
```

幾個語法細節：

- `${{ ... }}` — **expression**，evaluate 時會替換成實際值。`github.workspace` 是 repo 在 runner 上的絕對路徑；`secrets.FOO` 是 secret；`matrix.os` 是 matrix 變數
- `>> $env:GITHUB_ENV` — 寫入這個 GitHub 給的特殊檔，後續 step 開始時會被讀進該 step 的環境變數。這是**跨 step 傳遞 env var** 的標準手法
- `run:` 和 `uses:` 互斥：同一個 step 二選一
- 每個 step 是獨立 process，variable 不跨 step，但 `GITHUB_ENV`、`GITHUB_PATH`、job outputs 三個機制可以橋接

## GitHub UI 導覽

- **`Actions` tab**：列出所有 workflow + 每次 run 的歷史。點 run → 點 job → 點 step 看 log
- **PR 頁面的 `Checks` tab**：這個 PR 觸發了哪些 workflow、狀態為何、紅/黃/綠一眼看到
- **Artifacts**：在某次 run 詳情頁的**最底部**。下載後是 zip
- **「Re-run jobs」按鈕**：run 頁面右上角。選「Re-run failed jobs」只重跑掛掉那幾個 job
- **Cancel workflow**：跑到一半看出有問題可以按（會停止後續 step、浪費配額減半）
- **狀態徽章**：workflow 頁面 `...` → `Create status badge`，貼到 README 秀綠勾（私人專案拿來自己看狀態也方便）

## 常踩的坑

- **Workflow 沒跑** — 檢查檔案路徑對不對（`.github/workflows/*.yml`，`workflow` 不是 `workflows` 單數複數別寫錯），YAML 縮排有沒有漂移（YAML 對縮排非常敏感，用**空格不要用 tab**），`on:` 寫的事件有沒有對上你做的動作
- **Fork PR 拿不到 secret** — GitHub 故意的防惡意 PR 偷 token。要讓 fork PR 做需要 secret 的事得用 `pull_request_target`，但這是**已知的安全地雷**（會在 base repo context 執行，可被 compromise），一般不建議。多數開源專案接受這個限制
- **Cache 沒命中** — 看 step log 的 `Cache not found for input keys:`。key 裡有隨機值（時間戳之類）就每次都 miss；改用 `hashFiles('...')` 綁檔案 hash 比較穩
- **第一次跑特別慢** — 正常，cache 還沒 populate，deps 要從零 build。之後快
- **Debug logging** — Repo Settings → Secrets and variables → Actions → 新增 secret `ACTIONS_STEP_DEBUG` = `true`。下次 run 會印內部 debug 訊息，排查「為什麼 action 行為跟 doc 寫的不一樣」時很有用
- **`permissions:` 欄位** — `GITHUB_TOKEN` 的預設權限視 repo / org 設定，有些 repo 給的是 restrictive default（幾乎不能寫）。當 workflow 要發 release、修 issue、push tag、留 PR 評論時通常要顯式加：
  ```yaml
  permissions:
    contents: write      # 發 release / push tag 需要
    pull-requests: write # 留 PR 評論需要
    issues: write
  ```

## 進階（有需要再看）

- **Matrix strategy**：同 job 展成多平台 / 多版本平行跑
  ```yaml
  strategy:
    matrix:
      os: [ubuntu-latest, windows-latest]
      python: ['3.10', '3.11']
    fail-fast: false     # 一個 fail 不要把其他平行 job 殺掉
  runs-on: ${{ matrix.os }}
  ```
  會跑 2×2 = 4 個 job
- **Reusable workflow**：一個 workflow 呼叫另一個（`uses: ./.github/workflows/shared.yml`），參數化共用步驟
- **Composite action**：把幾個 step 打包成一個你自己的 action，放在 repo 的 `.github/actions/<name>/action.yml`
- **Self-hosted runner**：自己的機器當 runner，用於需要特殊硬體（GPU、內網）或機敏資料不想上雲時。注意安全：fork PR 跑到 self-hosted runner 上是嚴重漏洞
- **Environments + approval**：deploy 到 production 時要人工核准才往下走

## 參考

- 官方文件：https://docs.github.com/actions
- Workflow YAML 完整語法：https://docs.github.com/actions/reference/workflow-syntax-for-github-actions
- Marketplace：https://github.com/marketplace?type=actions
- 各 runner 預裝軟體清單：https://github.com/actions/runner-images（`images/windows/` 或 `images/ubuntu/`）
- 本專案怎麼用 GitHub Actions：`CI.md`
