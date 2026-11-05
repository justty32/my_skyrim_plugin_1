# 原生 DialogueMenu spike — 執行期驅動 Skyrim 原生對話選單的可行性分析

> 探索性可行性分析，2026-05-24。由 read-only research agent 產出。所有引擎面論斷均對照本 repo
> vendored 的 `CommonLibSSE-NG/include/RE/` 標頭逐一查證並標出真實路徑/行號／真實簽名。
> **僅分析，未動任何既有檔案**（`src/`、`cmake/`、`plugin.cpp`、所有 `.md` 一律不碰；本檔為唯一新增）。
> 與本專案 ethos 一致（`CLAUDE.md`、memory `project_quest_engine`）：
> **不開 Creation Kit、不做 ESP/ESL，全部在執行期用 C++ 程式碼。**
> 交叉引用：
> - `QUEST_ENGINE_DESIGN.md` §5（既有的 spike 研究筆記、go/no-go 準則）— 本檔是它的 header-verified 深化版。
> - `research/PROCGEN_NPC_FORMS.md`（**同一族**的「執行期鑄造 form / 不能跨存檔持久」問題與 co-save 食譜模式先例）。
> - `research/ALCHEMY_SPIKE_FINDINGS.md`（動態 form 持久化的同類硬傷）。
> - `src/skyrim/dialogue/MessageBoxPresenter.{h,cpp}`、`src/core/Ports.h`、`src/core/QuestEngine.{h,cpp}`、`src/skyrim/SkyrimAdapter.cpp`（保底 backend 與 resumable presenter 的現行接線）。

---

## 0. 要回答的問題

能否讓 SKSE plugin 在**執行期**、**不靠 CK/ESP 作者預先寫好的 `TESTopic`/`TESTopicInfo`**，
驅動 Skyrim 的**原生對話選單**（真正那個 topic-based 對話 UI：上方 NPC 台詞 + 下方可點選 topic 清單），
把我們 JSON quest engine 的 `presentNode(speaker, lines, choices)` 接到它上面、且玩家點選後能回呼到
`QuestEngine::submitChoice(idx)`？對照保底方案：`MessageBoxPresenter`（已實作、幾乎無風險）。

引擎的 dialogue 已經是 **resumable** 的：`presentNode` 只負責顯示（display-only、不阻塞），
引擎靠 `submitChoice()` 推進（`src/core/Ports.h:20-27`、`src/core/QuestEngine.h:75-80`）。
所以這題**不需要**改 core 狀態機，只需要一個新的 `IDialoguePresenter` 實作。

---

## 1. 結論先講（TL;DR / VERDICT）

| 問題 | 結論（header-verified） |
|---|---|
| 原生對話相關 RE 類別是否存在、layout 是否摸得到？ | **是**。`DialogueMenu`、`MenuTopicManager`（含 `Dialogue` 內部結構、`dialogueList`、`speaker`）、`TESTopic`、`TESTopicInfo`、`DialogueItem`（**有 public ctor**）、`DialogueResponse`（`text`/`voice` 欄位）全部齊備、static_assert 過、有 RELOCATION_ID。 |
| `TESTopic`/`TESTopicInfo` 能否執行期鑄造（不靠 ESP）？ | **能鑄**。`ConcreteFormFactory_TESTopic_75_` / `ConcreteFormFactory_TESTopicInfo_76_`（甚至 `ConcreteFormFactory_TESQuest`）都在 VTABLE/RTTI 清單裡 → `IFormFactory::GetConcreteFormFactoryByType<TESTopic>()` 可用，與本 repo `NpcGenerator::SpawnNpc` 鑄 `TESNPC`、鑄 `SpellItem` 同一條成熟路徑。**但「鑄得出」≠「引擎會撈出來顯示」。** |
| 引擎會不會把執行期塞的 topic 撈進選單？ | **沒有現成、受支援的入口**。選單內容由 `MenuTopicManager` 依 branch/subtype/`saidOnce`/條件過濾後產生 `Dialogue` 清單，再透過 Scaleform GFx (`FxDelegate`) render。要讓自製 topic 出現，得**要嘛**把整棵 quest/branch/topic/info 樹接好讓 vanilla 過濾通過（極脆弱），**要嘛**直接 hook 引擎的 populate 函式 + 手動塞 `MenuTopicManager::dialogueList`（需逆向找位址、trampoline hook，純 R&D）。 |
| 持久化（save/reload）？ | **破**。動態鑄的 `TESTopic`/`TESTopicInfo`/`TESQuest` 拿 `0xFF......` 動態 FormID，**不能可靠跨存檔存活**（與 `PROCGEN_NPC_FORMS.md` §3、`ALCHEMY_SPIKE_FINDINGS.md` 同一硬傷）。不過對「短暫開一段對話」這用途，持久化其實**不是必要**——對話是 transient 的。真正的持久化責任在我們**自己的 engine progress blob**（已實作，見 §6），不在引擎的 form。 |
| 語音 / 字幕 / lip？ | **無語音是已知硬傷**。`DialogueResponse` 無 voice 檔時引擎傾向直接跳過台詞。`TOPIC_INFO_FLAGS::kForceSubtitle` + `kNoLIPFile` 可緩解（強制顯示字幕、不要 lip 檔），但這只在「走引擎正常 say 流程」時生效；走純 hook/記憶體注入則要自己管 `SubtitleManager`。 |
| **VERDICT** | **維持 MessageBox 保底。** 原生 DialogueMenu 在 header 層級「摸得到」，但**沒有任何受支援的執行期 API 能在不寫 ESP 的情況下開一段自訂對話**；唯一可行路徑是 trampoline hook + 記憶體注入 + 自管字幕/語音，屬高風險、跨遊戲版本（SE/AE/GOG/VR）位址各異、且回報極低的 R&D。**建議只做 §5「最小原生實驗」驗證可行性訊號，不上 production。** |

**一句話**：能鑄 form、結構摸得到，但「把它變成玩家看得到、點得到、會回呼的對話」沒有食譜，只有逆向工程；
MessageBox 已經滿足 go/no-go 準則（自訂文字 + ≥2 可點選項 + 點選回呼 C++），**沒有非做原生不可的理由**。

---

本分析已按主題拆分：

- [相關 RE 類別與真實簽名](native_dialogue_spike_re_classes.md)：`DialogueMenu`、`MenuTopicManager`、`TESTopic`／`TESTopicInfo`、`DialogueItem`、字幕等類別的查證。
- [執行期注入與風險](native_dialogue_spike_injection_risks.md)：哪些可鑄、哪些需作者 form、save/reload 與語音限制，以及風險評估。
- [VERDICT、presenter 接法與缺口](native_dialogue_spike_verdict_presenter.md)：VERDICT 與最小原生實驗、resumable presenter 接法、待逆向缺口。
