# BRANCHES — 分支盤點與處置

盤點日 **2026-08-02**。每條分支都**實際交叉編譯過**（`VCPKG_ROOT=~/vcpkg`、preset `build-release-clang-cl-linux`、clang-cl on Manjaro），結果如下表——不是憑 commit 訊息猜的。

## 一覽

| 分支 | 最後 commit | 日期 | vs `main` | 編得過? | 處置 |
|---|---|---|---|---|---|
| `main` | `9db9898` | 2026-06-06 | — | ✅ 出 `DaylightDungeon.dll` | **主線**。當前產品＝DaylightDungeon（FollowLight 點光源 + AmbientBoost cell 環境光 + NpcGenerator）|
| `feature/court-wizard` | `992d7ab` | 2026-05-26 | +34 / −3 | ✅ | **留著**。整套 quest engine + Skyrim adapter + procgen，實機驗證過；文件已撈上 main（見下）|
| `feat/custom-weapon` | `9e17952` | 2026-04-23 | +2 / −21 | ✅ | **可刪**。原始碼已存 [`vendor/custom-weapon/`](vendor/custom-weapon/) |
| `feat/magic-toolkit` | `97c5d10` | 2026-04-26 | +13 / −7 | ❌ 12+ 錯 | **可刪**。原始碼已存 [`vendor/magic-toolkit/`](vendor/magic-toolkit/)，壞法已記錄 |
| `feat/power-shout` | `343b141` | 2026-04-23 | +2 / −21 | ❌ 2 錯 | **可刪**。原始碼已存 [`vendor/power-shout/`](vendor/power-shout/) |
| `feat/npc-generator` | `da28f92` | 2026-04-24 | +0 / −16 | （＝main 子集）| **可刪**。已完全併進 `main`，零獨有 commit |
| `feat/follow-light-ambient` | `9db9898` | 2026-06-06 | +0 / −0 | ✅ | **可刪**。與 `main` **同一個 commit**，純重複 |

> `feat/magic-toolkit` / `feat/power-shout` 編不過**不是邏輯壞掉，是 CommonLibSSE-NG 的 `RE::` 介面在 2026-04 之後動過**（`Actor` 的 actor-value 方法搬去 `ActorValueOwner`、`EffectSettingData::magicSkill` 改名、enum 少了 `|` operator…）。逐條修法列在 [vendor/README.md](vendor/README.md)。

## 這輪做了什麼

**沒有刪任何分支**（本機或 remote 都沒動）——只是把值得留的東西撈到 `main`，讓刪除變成零損失的動作。

從 `feature/court-wizard` 撈上 `main`：

- **12 份 `research/*.md`**（3205 行，全部對照 vendored 的 CommonLibSSE-NG 標頭查證過）：navmesh-free 尋路、NPC 行為深度改造、程序化生成（室內／室外／NPC form）、3D 物理煉金可行性 + spike 發現、`kPreSaveGame` dynamic form 清理、原生對話 spike、`spell_cast_on` hook、quest engine 稽核與 review findings。
- **`QUEST_ENGINE_SPEC.md` / `QUEST_ENGINE_DESIGN.md` / `config/schema/quest.core.schema.json`** ——court-wizard 版**嚴格較新且是超集**（同為 2026-05-23 但晚 30 分鐘～14 小時），已取代 main 舊版。多出：globals（§2.4）、`schedule`/`timer`、`reset_quest`、非同步訊息標準擴充（§4.5）、Clock 埠升級。
- **設計/測試/交接文件 + 兩份 HTML 儀表板** → [`archive/court-wizard/`](archive/court-wizard/README.md)（含 `progress.md` 的過時警告——它裡面的 ModForge 進度是 It.7–It.10 時代的，早就不準）。

從三條 spike 分支撈進 [`vendor/`](vendor/README.md)：`MagicToolkit.{h,cpp}`、`power_shout.{h,cpp}`、`custom_weapon.{h,cpp}`。**不參與建置**（`CMakeLists.txt` 不 glob，也沒登記進 `cmake/sourcelist.cmake`）。

## 要收掉分支的話

五條可刪的都已無獨有價值。SHA 記在上表，真要救回來 `git branch <name> <sha>` 即可。

```bash
git push origin --delete feat/follow-light-ambient feat/npc-generator feat/custom-weapon feat/magic-toolkit feat/power-shout
```

```bash
git branch -D feat/follow-light-ambient
```

`feature/court-wizard` **不要刪**——它是一整條還活著的平行產品線，程式碼只在那邊。
