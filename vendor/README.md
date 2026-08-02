# vendor/ — 抽出保存的分支原始碼（**不參與建置**）

從已無人維護的 feature 分支撈出來的實作，留作**參考／回收料**。`CMakeLists.txt` 不 glob，這裡的 `.cpp` 沒登記進 `cmake/sourcelist.cmake`，所以**不會被編**——要用就把檔案搬進 `src/` 並補登記。

分支盤點與各自的取捨理由見 [BRANCHES.md](../BRANCHES.md)。

| 目錄 | 來源分支 | 2026-08-02 實編結果 | 一句話 |
|---|---|---|---|
| [`magic-toolkit/`](magic-toolkit/) | `feat/magic-toolkit`（`97c5d10`） | ❌ 編不過（API 漂移） | 144 個 Lesser Power 法術的 runtime 實作目錄 |
| [`power-shout/`](power-shout/) | `feat/power-shout`（`343b141`） | ❌ 編不過（API 漂移） | 動態生成三詞龍吼 |
| [`custom-weapon/`](custom-weapon/) | `feat/custom-weapon`（`9e17952`） | ✅ 編得過 | 載入時在玩家面前掉一把動態生成的測試劍 |

## 為什麼 magic-toolkit / power-shout 編不過

兩者都寫於 2026-04，之後 CommonLibSSE-NG 的 `RE::` 介面動過，**不是邏輯壞掉，是 API 漂移**。實測（`clang-cl` 交叉編譯，preset `build-release-clang-cl-linux`）的錯誤：

**`magic-toolkit`（12+ 錯）**

| 錯誤 | 現況 |
|---|---|
| `RE::Actor` 沒有 `GetActorValue` / `SetActorValue` / `GetPermanentActorValue` / `RestoreActorValue` | 這組已移到 `RE::ActorValueOwner`（`Actor` 繼承它，改走 `actor->AsActorValueOwner()->...`） |
| `ObjectRefHandle*` 用 `.` 取成員（`MagicToolkit.cpp:209`） | 該處拿到的是指標，要 `->` |
| `auto* ref = <NiPointer<TESObjectREFR>>`（`:222`） | `NiPointer` 不是裸指標，接成 `auto ref =` 再 `.get()` |
| `NiPoint3[3]` → `RE::NiPoint3` 無轉換（`:629`） | 取陣列元素或改用 `NiMatrix3` |
| 呼叫少一個引數（`:641`） | 該 API 已加參數 |

**`power-shout`（2 錯）**

| 錯誤 | 現況 |
|---|---|
| `EffectSetting::EffectSettingData` 沒有 `magicSkill`（`power_shout.cpp:48`） | 欄位改名／改位置，查 `MagicItem`／`EffectSetting` 標頭 |
| `Flag \| Flag` 沒有 operator（`:57`） | 該 enum 現在需 `stl::enumeration<Flag>` 或顯式 cast |

要復活哪一支，照上表逐條改即可；`custom-weapon` 現況直接可用。
