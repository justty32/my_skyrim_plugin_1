# GitHub Actions 入門

GitHub Actions 是 GitHub 內建的 CI/CD 服務：`.github/workflows/*.yml` 監聽事件，並在 runner 上依序執行工作。本專案的 workflow、vcpkg cache key、artifact 與 rename 契約，以 [`CI.md`](CI.md) 為唯一正典；本文只解釋通用概念。

## 核心概念

| 詞 | 解釋 |
|---|---|
| **Workflow** | 一個 YAML pipeline，存放於 `.github/workflows/*.yml` |
| **Event / Trigger** | 觸發條件，例如 `push`、`pull_request`、`schedule`、`workflow_dispatch` |
| **Job** | 在單一 runner 上執行的一組 steps；多個 job 預設平行 |
| **Step** | Job 內的循序動作；失敗通常會使 job 失敗 |
| **Runner** | 執行 job 的 GitHub-hosted 或 self-hosted 機器 |
| **Action** | 以 `uses: author/name@version` 引用的可重用 step |
| **Artifact** | Run 產出的可下載檔案 |
| **Cache** | 依 key 跨 run 復用的依賴或建置資料 |
| **Secret** | Repo 設定中加密保存的值，以 `${{ secrets.NAME }}` 引用 |
| **`GITHUB_TOKEN`** | 每次 run 自動建立的短效 token；寫入權限須用 `permissions:` 明確開啟 |

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

Push 後可在 Actions tab 查看 workflow、job、step 與 log。

## 常見觸發方式

```yaml
on:
  push:
    branches: [main]
  pull_request:
  workflow_dispatch:
  schedule:
    - cron: '0 3 * * *'  # UTC
```

只在 tag push 時執行：

```yaml
on:
  push:
    tags: ['v*']
```

## Action 與版本 pin

常用 action：

- `actions/checkout@v4`：checkout repo。
- `actions/cache@v4`：儲存與還原 cache。
- `actions/upload-artifact@v4`：上傳 artifact。
- `ilammy/msvc-dev-cmd@v1`：設定 MSVC 開發環境。

日常可 pin major（例如 `@v4`）以接收同 major 修補；高安全性環境可 pin commit SHA。避免 `@main` 或 `@master`，以免上游未經審查的變動直接改變 CI 行為。

## YAML 與 expression

- `${{ ... }}` 是 GitHub expression，例如 `${{ github.workspace }}`、`${{ matrix.os }}`。
- `run:` 與 `uses:` 在同一個 step 互斥。
- 每個 step 是獨立 process；跨 step 傳值用 `GITHUB_ENV`、`GITHUB_PATH` 或 job outputs。
- YAML 對縮排敏感，使用空格而非 tab。
- Fork PR 預設拿不到 secret；不要為了繞過限制隨意改用高風險的 `pull_request_target`。

需要讓 `GITHUB_TOKEN` 寫入時，依實際用途給最小權限：

```yaml
permissions:
  contents: write
  pull-requests: write
```

## UI 與排錯入口

- **Actions tab**：查看 workflow 與 run 歷史。
- **Run → job → step**：查看實際 log，先找第一個失敗 step。
- **PR Checks**：查看 PR 觸發的 checks。
- **Artifacts**：在 run 詳情頁下載產物。
- **Re-run failed jobs**：只重跑失敗 job。
- **Cancel workflow**：停止不需要繼續的 run。

常見問題：

- Workflow 沒跑：檢查 `.github/workflows/*.yml` 路徑、YAML 縮排與 `on:`。
- Cache 沒命中：查看 `Cache not found for input keys:`，避免把時間戳等隨機值放進 key。
- Fork PR 缺 secret：這是安全邊界，不是 runner 故障。
- 權限不足：核對 `permissions:` 與 repo / org 的預設權限。

## 進階機制

- **Matrix strategy**：同一 job 展開成多平台或多版本。
- **Reusable workflow**：以 `uses: ./.github/workflows/shared.yml` 呼叫共用 workflow。
- **Composite action**：把多個 steps 封裝在 `.github/actions/<name>/action.yml`。
- **Self-hosted runner**：使用自有機器；不要讓不受信任的 fork PR 直接執行。
- **Environment approval**：部署前加入人工核准。

## 參考

- 官方文件：https://docs.github.com/actions
- Workflow 語法：https://docs.github.com/actions/reference/workflow-syntax-for-github-actions
- Marketplace：https://github.com/marketplace?type=actions
- Runner 映像：https://github.com/actions/runner-images
- 本專案 CI 正典：[CI.md](CI.md)
