# testing — 跑測試 / 驗證

[WORKFLOWS](../WORKFLOWS.md)｜[INDEX](../INDEX.md)

改完怎麼確認沒壞：有哪些驗證、各自的指令、哪些 agent 自己跑得了、哪些得交給使用者。

**何時用**：改完程式要驗、使用者說「跑測試」、重構前要記基準。
**何時不用**：環境還沒裝好、不知道指令哪來 → [dev-env](dev-env.md)；驗證紅了要查成因 → [investigation](investigation.md)。

## Done when

- 下表標「改完必跑」的列指令回傳 0。
- 「誰跑」是使用者的列，已在 [WAIT_USER](../../../../WAIT_USER.md) 各留一行（寫明跑什麼、什麼算過）。

## 驗證表

| 驗證 | 指令 | 誰跑 |
|------|------|------|
| 打包契約（改 `pack.sh`／`pack.ps1` 時必跑，跑對應那支）| `bash scripts/test_packaging.sh`（Linux）／`scripts/test_packaging.ps1`（Windows）| agent |
| quest-PRF 契約（改 `src/core/DeterministicRandom` 或 `QUEST_ENGINE_SPEC.md` 附錄 B 時）| `bash scripts/test_quest_prf.sh`（需 `g++`）／`scripts/test_quest_prf.ps1` | agent |
| 編譯驗證（改 `src/` 必跑）| `cmake --build build/release-clang-cl-linux`（Linux）；Windows CI 跑 `build-release-msvc` | agent |
| lint / 型別 | 無設定；C++ 沒有測試套件（`CMakePresets.json` 的 `testPresets` 為空）| — |
| 實機（dll 載入、log 無 `[error]`／`[critical]`、功能目視）| 照 [FOR_AGENT.md](../../FOR_AGENT.md) §4–5 看 `DaylightDungeon.log` | 使用者 → [WAIT_USER](../../../../WAIT_USER.md) |

「誰跑」只有兩種值：**agent**（本機跑得動）、**使用者 → WAIT_USER**（要實機、外部服務、帳號、付費、目視）。判不準就當後者。

## 測試分類

本專案以「需不需要工具鏈／遊戲」分：`scripts/test_*.sh` 不需 MSVC／vcpkg，任何 Linux 機器可跑；編譯驗證需要 vcpkg＋clang-cl／xwin（或 Windows MSVC）；實機只有使用者能驗（見 [dev-env](dev-env.md) 的跨機表）。

- `fast`：本專案自己的 unit／source gate；每次小改都跑。
- `contract`：跨元件的真實 CLI／process 邊界；改 producer、consumer 或協定時跑。
- `full`：README 指定的完整離線 suite；commit 前或大改後跑。
- `external`：需要實機、外部服務、素材、帳號或人工感官驗收；agent 代跑不了的記到 [WAIT_USER](../../../../WAIT_USER.md)。

## 綠燈不等於有檢查

**一道檢查通過，可能是因為它根本沒在檢查。** 這不是假設：曾在一天內抓到四個恆真檢查——結構稽核、「鎖已釋放」的檢查指向已刪目錄、連結檢查器走到子 repo 指標就停、自製字元偵測。

**規則：新增或修改一道檢查時，要證明它能變紅。** 先餵一個**應該被擋**的輸入，確認 exit ≠ 0；再餵正確的輸入，確認 exit = 0。**沒做過這個雙向驗證的綠燈不算證據。**

兩個推論：檢查器的**涵蓋範圍要跟著結構走**——拆出子 repo、搬走目錄之後，回頭確認檢查器還看得到那些地方（見 [refactor/moving-things.md](refactor/moving-things.md)）；**靜態全過不等於畫面上是對的**，方框、亂碼、截斷、手感只有人眼看得出來，這類記到 [WAIT_USER](../../../../WAIT_USER.md)，不要自己宣稱通過。

## 交接

- 綠燈後回 [feature-dev](feature-dev/README.md) 接完剩下的步驟（code map → 文檔 → commit）。
- 同一個紅燈第二次撞到 → [common/gotchas](common/gotchas.md)。
