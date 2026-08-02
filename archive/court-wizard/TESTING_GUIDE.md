# 宮廷大法師 Suite — In-Game 測試手冊（逐步）

> 給「人類在 Skyrim 裏實測」用。整包是 Linux 交叉編譯、**從未在遊戲裏跑過**，所以這份手冊的目的就是把每個功能逐項驗證掉。
> 對應版本：`feature/court-wizard`，DLL = **2026-05-24 最新 build**（含「真實 `spell_cast_on` hook」取代舊 F10 假事件，加上 in-game 修正）。成果包 `dist/CourtWizardSuite-0.0.1.zip` 已刷新（新 DLL + demo 顯示文字全轉英文，避免無 CJK 字型時變方塊）。
> **本版熱鍵變更（重要）**：`F7` = 強制觸發排程計時器（舊 `F9`，因與快速讀檔衝突而改）、`F8` = 輪詢到期計時器、`F11` = 煉藥 spike（不變）；**舊 `F10` 假事件已移除**——解咒改成「真的對 NPC 施法」。
> 搭配閱讀：`使用說明書.md`（功能總覽）、`research/REVIEW_FINDINGS.md`（已知風險）。

---

## 怎麼用這份文件

- 從上到下做，**順序是刻意排的**：先確認 plugin 活著，再由低風險到高風險逐功能測。
- 每步有 ☐ 勾選框、**預期結果**、**怎麼算過/不過**。
- 失敗時看「失敗就回報」那行，把指定資訊貼給我，我據此修。
- **時間有限只測 3 件事**：→ 跳到最後「最小測試集」。

---

## 第 0 步：環境準備（只做一次）

☐ **0.1 確認前置**：Skyrim SE/AE/GOG + SKSE64，且**用 SKSE 啟動**（不是原版 exe / Steam 直接啟動）。
☐ **0.2 安裝**：把成果包 `Data/` 內容裝進去（MO2 當 mod，或複製進遊戲 `Data/`）。確認檔案到位：
  - `SKSE/Plugins/TemplatePlugin.dll`
  - `SKSE/Plugins/Template_Plugin/quests/`、`/procgen/`、`/schema/`
☐ **0.3 找到 log 檔**（之後每步都靠它）：
  `C:\Users\<你>\Documents\My Games\Skyrim Special Edition\SKSE\TemplatePlugin.log`
  （AE/GOG 路徑類似；檔名是 `TemplatePlugin.log`。建議開個文字編輯器一直開著它。）
☐ **0.4 準備測試角色**：建議用一個**乾淨測試存檔**，且角色**沒有任何煉金 perk**（為了煉藥數值比對，第 4 步會用到）。
☐ **0.5 學會主控台**：按 `~` 開 console。常用：
  - `help "名稱" 4` → 查某物的 FormID（驗證佔位 ID 用）
  - `coc whiterunorigin` → 快速傳送到白漫（做室內外測試方便）
  - 截圖鍵預設 `PrtScn`（或用 Steam 截圖）

> ⚠️ 重要心理準備：**程序生成的 recipe 目前用的部分 ID 是「中等信心」或佔位**（見 `使用說明書.md` 第 5 節）。如果某個物件 ID 在你的環境不存在，plugin 會在 log 記一筆警告並**略過該物件、不會崩**——所以「房間少了幾件東西」很可能是 ID 要換，不是 bug。第 6 步教你怎麼換。

---

## 第 1 步：冒煙測試（plugin 是否活著）

☐ **1.1** 啟動遊戲 → 讀入任一存檔（或新遊戲）到可操作狀態。
☐ **1.2** 看 `TemplatePlugin.log`，應出現類似：
  - `Plugin loaded`
  - `kDataLoaded: game data loaded`
  - 劇情引擎載入字樣（如 `loaded quest 'demo_court_wizard'`）

**算過**：遊戲沒在載入時崩潰，且 log 有上述字樣。
**失敗就回報**：把 `TemplatePlugin.log` 整份貼給我 + 說明在哪一刻崩（主選單／讀檔／進遊戲後）。

---

## 第 2 步：取得測試法術（後面好幾步都要用）

新遊戲或讀檔時，plugin 會自動把這些加給玩家（lesser power 或法術）：
`C++: Generate Room`、`C++: Conjure Keep`、`C++: Rearrange Furnishings`、`C++: Conjure NPC`、`C++: Conjure Item`

☐ **2.1** 開魔法選單 → 找 **Powers（能力）** 與 **Spells（法術）** 兩個分類，確認有上面這些 `C++:` 開頭的項目。
☐ **2.2** 若**完全找不到**：可能是新遊戲才加。試「新遊戲」走到能操作，再看；或讀檔後存檔再讀一次（觸發 `kPostLoadGame` 重加）。

**算過**：至少看得到這幾個 `C++:` 法術/能力。
**失敗就回報**：log 裏搜 `GiveSpellsToPlayer` / `InitializeSpells` 的行貼給我。

> 施放方式：**能力(Power)** → 裝到「能力」槽，按喊聲鍵（預設 `Z`）施放；**法術(Spell)** → 裝到手上，按施法鍵。哪個是哪個看魔法選單分類。

---

## 第 3 步：劇情引擎 + 白漫召喚循環（核心）

> 實際運行的 quest 是 **`cw_whiterun_summons.json`**（白漫召喚循環，可重複、有進度），**不是** `demo_court_wizard.json`（後者只是參考檔）。下面照真實流程寫。

☐ **3.0 前置（重要）**：接任務的對話選項**卡在「玩家會 Turn Lesser Undead」**這個條件——沒有就只看得到「問報酬 / 婉拒」。最快：console 打 **`player.addspell 0004B146`**（Turn Lesser Undead；或正常去學該法術書）。〔此閘門原本是占位 ID `0x000211EF`，已修成已驗證的 `0x0004B146`。〕
☐ **3.1** 進遊戲後，**等 24 遊戲小時**（找床睡覺最快），**或**直接按 **`F7`**（強制觸發所有排程計時器，免等。舊 `F9` 已改 `F7`）。
☐ **3.2** 預期：收到信件「**A Letter from Jarl Balgruuf**」+ 一個地圖標記，接著彈出 **MessageBox 對話**（Jarl Balgruuf 說府上一名侍從中咒），三個選項。
☐ **3.3** 選 **「Let me see the retainer. (I have learned the spell to turn the undead.)」**（**只有你有 Turn Lesser Undead 才會出現**）→ 預期：Jarl 播 idle、面前生成 NPC「**Afflicted Retainer**」、你被**傳送到她身邊**、目標 `lift_curse` 變 active。〔另兩選項：「問報酬」先說明可選 金/附魔物/龍吼，再回到接受或婉拒；「還沒學會」則婉拒，過 24h 會再召喚。〕
☐ **3.4** **對著 Afflicted Retainer 施放任一法術**（本版 `F10` 假事件已移除，改真實 `TESMagicEffectApplyEvent` hook：任何魔法效果命中該 NPC 即觸發 `spell_cast_on{victim}`）。手上沒法術就裝個破壞系（如 Flames）對準她放。→ 預期：放個音效 + 目標完成 → 彈出對話「**The curse is broken! ... What reward will you have?**」。
☐ **3.5 選獎勵**：三選一 →「Gold will do.」得 500 金 /「enchanted item」給一件物品 /「Word of Power」（需先前已完成 ≥1 次才出現）給龍吼 → 接「Whiterun will remember this.」→ 選「await your next summons」任務計數 +1 並重置（過 24h 再召喚）；累計完成 ≥2 次後會多出「成為常駐法師」選項。

**算過**：對話正常彈出、按鈕**對應正確**（按第 N 個就走第 N 個分支）、NPC 有生成、選的獎勵有拿到、重置後可再跑一輪。
**特別注意 1（按鈕對應）**：按鈕索引對應是程式裏唯一沒法靜態驗證的點（見 REVIEW M-項）——若**按了某個按鈕卻走了別的分支結果**，這是 bug，務必回報。
**特別注意 2（占位 FormID）**：除了上面已修好的閘門法術，這支 quest 還有幾個**占位 FormID 尚未查證**：地圖標記 `0x00018A52`、附魔物獎勵 `0x0001CB36`、龍吼 `0x00013E22`、音效 `0x0003EB4E`、Jarl idle `0x00013452`。**過不了只會在 log 記警告並 skip、不會崩**——若「沒地圖標記 / 選了附魔物卻沒拿到 / 沒學到龍吼 / 沒音效 / Jarl 沒動作」，那是已知占位問題，把 log 警告行貼我，我換成真值即可。
**失敗就回報**：哪一步沒反應 / 對話沒出 / 按鈕錯位，+ log 對應片段。

---

## 第 4 步：煉藥 spike（按 F11）

☐ **4.1** 用**無煉金 perk** 的角色，按 **`F11`**。
☐ **4.2** 看 log 找 `==== ALCHEMY SPIKE RESULT ====` 區塊（會列效果、強度 magnitude、持續 duration、金價）。
☐ **4.3** **數值比對**：到任一煉金台，手動選「藍山花 + 小麥」（共享「恢復生命」），看 vanilla 選單預覽的數值，跟 log 裏的數字比。**在 2~3 個不同煉金技能等級各比一次**。
☐ **4.4** 開背包確認多了那瓶藥水，可以喝。
☐ **4.5 存檔持久化**：喝之前先**存檔 → 讀檔**，確認藥水還在、還能喝。

**算過**：log 數值與 vanilla 選單**一致**，且藥水存讀後還在。
**失敗就回報**：log 的 RESULT 區塊 + vanilla 選單的截圖（兩邊數字），或讀檔後藥水消失/變空白。

---

## 第 5 步：runtime 生成（四種，重點戲）

> 共通：每種都先「生成 → 檢查當下」再「存檔 → 讀檔 → 確認重現」。重建會在 log 印 `rebuilt ...`。

### 5A. 生成物品（Conjure Item）— **優先測這個，剛修過累加 bug**
☐ 施放 `C++: Conjure Item` → 預期背包多一把「C++ Conjured Blade」（鐵劍變體，傷害 25 / 價值 500）。裝備它，看模型、揮砍音效是否正常。
☐ **存檔 → 讀檔** → 確認劍**還在**。
☐ **關鍵：重複「存檔→讀檔」2~3 次**，確認背包裏的劍**數量不會越來越多**（H1 修復重點）。
**算過**：劍可用，且多次存讀**不累加**。**失敗回報**：數量有增加 / 讀檔後變空白物品 → log 搜 `ProcgenItem`。

### 5B. 生成 NPC（Conjure NPC）
☐ 施放 `C++: Conjure NPC` → 面前出現一個 NPC（土匪模板）。log 記 base `0xFF...` id。
☐ **存檔 → 讀檔** → 確認 NPC 在原處重現（log `ProcgenNpc: rebuilt ...`），且**沒有變成兩個**。
**算過**：NPC 重現、不重複、不消失。**失敗回報**：log `ProcgenNpc` 片段 + 現象。
**已知限制**：重現的是「全新實例」，不是同一個 ref（邏輯持久化）——這是預期，不是 bug。

### 5C. 生成房間（Generate Room）
☐ 找個空地（室內外都行），施放 `C++: Generate Room` → 面前約一段距離出現房間（地毯/牆/床/桌椅/燭光/雜物）。
☐ **檢查 log** 有沒有 `skip`/`unknown form`/`warning` —— 有的話表示某些 recipe ID 在你環境不存在（見第 6 步換 ID）。
☐ 試著**坐椅子 / 睡床**（家具 marker 測試）。
☐ 施放 `C++: Rearrange Furnishings` → 家具換位置。
☐ **存檔 → 讀檔** → 房間在原處重現。
**算過**：至少**部分**家具/牆出現且可互動、存讀後重現。**失敗回報**：截圖（缺什麼/浮空/重疊）+ log 警告行。
**已知**：vanilla 室內套件只有 CK 裏才完美對齊，所以**牆對不齊/有縫/家具位置怪**是預期內、屬「需調 recipe」而非崩潰級 bug。

### 5D. 生成城堡（Conjure Keep）
☐ **到室外**，施放 `C++: Conjure Keep` → 面前出現模組城堡（牆+塔+門+主堡）。
☐ 確認**貼在地面**（不浮空/不陷地）、玩家**走得上去**（有碰撞）。
☐ **存檔 → 讀檔** → 城堡重現。
**算過**：城堡出現、貼地、可走、存讀重現。**失敗回報**：截圖 + log。
**已知限制（不是 bug，別當 bug 報）**：① 走遠（出 5×5 格）城堡會**消失**、無遠景 LOD、有 pop-in；② NPC **無法**在城堡上自動尋路（沒有 runtime navmesh）；③ 室內施放會在 log 中止。

---

## 第 6 步：劇情進度持久化（'QEST'）

☐ **6.1** 照第 3 步推進到一半（例如目標 `lift_curse` 已 active、或召喚計時器還沒到）。
☐ **6.2** **存檔 → 退到主選單 → 讀回該檔**。
☐ **6.3** 看 log：`OnLoad staged ... 'QEST'` → `RebuildStaged applied 'QEST' progress + globals`。
☐ **6.4** 確認：目標仍 active、金幣/全域計數保留、之後該觸發的計時器仍會觸發、對話能接續。

**算過**：讀檔後任務從原進度續跑，沒從頭開始、也沒重跳開場訊息（M2 修復重點）。
**失敗就回報**：讀檔後任務重置 / 又跳開場白 / 目標歸零 → log 的 `QEST` 相關行。

---

## 第 7 步：用 JSON 驅動程序生成（進階，選測）

☐ **7.1** 把 `Template_Plugin/quests/demo_procgen.json` 或 `demo_gen_npc.json` 設成引擎載入目標（目前預設載 `demo_court_wizard.json`；要改載哪支需小調整，或在你自己的劇情 JSON 的 `do` 裏呼叫 `generate_interior`/`generate_structure`/`generate_npc`/`generate_item` 動作）。
☐ **7.2** 觸發後確認對應的東西被生成（同第 5 步檢查）。
**這步偏開發向**，不急；先把第 1~6 步測完。

---

## 換掉「找不到」的 FormID（第 5 步出現 skip 警告時）

1. console 打 `help "Tankard" 4`（或 log 警告裏提到的名字）→ 看有沒有列出 + 它的 8 碼 FormID。
2. 編輯對應 recipe（`Template_Plugin/procgen/recipe_cottage.json` / `recipe_keep.json` / `recipe_npc.json` / `recipe_item.json`），把該 `"form"` 換成 `"0x<FormID>~Skyrim.esm"` 格式（master 用該物件實際所屬的 esm）。
3. 存檔、重進遊戲再試。
> 哪些 ID 信心較低、來源為何，看 `research/REVIEW_FINDINGS.md` 和 agent 的 FormID 表。

---

## 失敗 / 崩潰怎麼回報給我（很重要）

每筆回報請附：
- **哪一步**（編號）+ **預期 vs 實際**
- **`TemplatePlugin.log` 的尾段**（崩潰前最後 ~30 行最關鍵）
- 視覺問題附**截圖**
- 崩潰的話：**重現步驟**（按了哪個鍵/法術的當下崩）

我拿到 log 就能定位是哪個模組（log 行首通常有 `ProcgenItem` / `ProcgenNpc` / `Procgen` / `QEST` / adapter 等標籤）並修。

---

## 最小測試集（時間有限就測這 3 件）

1. **第 1 步**冒煙（plugin 活著、不崩）——最基本。
2. **第 5A**生成物品 + 多次存讀不累加——剛修的高風險 bug。
3. **第 3 步**劇情對話流程（含按鈕對應）——核心、且按鈕對應無法靜態驗證。

這三項過了，代表「載入、生成+持久化、對話」三大支柱都活著；其餘是廣度與打磨。

---

## 測完之後的下一步（依結果決定）

- **大致能跑** → 回報哪些過/不過，我據 log 修 in-game 才現形的問題；接著可挑：把煉藥 spike 補 co-save、procgen 換成一套真正對齊的模組化套件、把 ModForge（ESP 產生器）接進這條 pipeline、或做新功能。
- **某模組崩** → 先給我那模組的 log，我修掉再請你重測該項。
- **只是 FormID/幾何不對** → 多半你自己換 recipe ID 就好（見上節）；要我幫你找一套對齊的 vanilla 套件也行。
