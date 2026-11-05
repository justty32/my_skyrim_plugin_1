# 執行期程序化生成 NPC（Form 層級）— SKSE C++ 可行性與設計分析

> 探索性可行性分析，2026-05-23。由 research agent 產出。所有引擎面論斷均對照本 repo vendored 的
> `CommonLibSSE-NG/include/RE/` 標頭逐一查證並標出真實路徑/行號。**僅分析，未動 plugin 程式碼**
> （`src/`、`cmake/`、`plugin.cpp` 一律不碰）。
> 與本專案 ethos 一致（`CLAUDE.md`、`MODDING_COOKBOOK.md`、memory `project_quest_engine`）：
> **不開 Creation Kit、不做 ESP/ESL，全部在執行期用程式碼。**
> 交叉引用：`research/ALCHEMY_SPIKE_FINDINGS.md`（動態 `AlchemyItem` 的**同一族**持久化問題）、
> `research/NPC_BEHAVIOR_DEEP_MODIFICATION.md`（actor 存在後的 AI/package）、
> `research/PROCGEN_INTERIOR.md`（「存種子+食譜、讀檔重建」hybrid 模式的先例）。

---

## 核心問題（使用者的原話）

> SKSE plugin 能否在執行期**在 Form 層級程序化生成一個 NPC**——一個真正的、能存進存檔的
> `TESNPC_` / `TESActorBase` form，由 C++ 鑄造出來，而**不只是把既有 base form 暫時擺一個 reference**？

使用者明確對比「**暫時的**」（temporary placed ref）vs「**Form**」（真正的 base form）。整份報告的成敗在兩件事：
**(a)** 能不能在執行期鑄造一個新的 `TESNPC_` form？
**(b)** 它能不能正確跨 save/reload 存活、且保有穩定身分（像 ESP 作者寫的 NPC 那樣）？

社群普遍認為「沒有 ESP/ESL 就做不到持久的 NPC」。以下逐點查證，並解釋**為什麼**。

---

## 結論先講（TL;DR）

| 問題 | 結論 |
|---|---|
| (a) 能否執行期鑄造一個 `TESNPC_` form？ | **能**。`IFormFactory::GetConcreteFormFactoryByType<TESNPC>()` 存在且可用；本 repo `NpcGenerator::SpawnNpc` **已經這樣做**。鑄出來的是真 form，拿到 `0xFF......` 動態 FormID，當下 session 完全可用。 |
| (a') 能否複製既有 base 再改？ | **能，且更務實**。`TESForm::CreateDuplicateForm` 對 `TESNPC` 並未被覆寫成 `{return 0;}`（不像 `CELL`），可走預設實作；或更穩的 `factory->Create()` + `newBase->Copy(template)`（本 repo 現行做法）。複製品同樣得到 `0xFF......` FormID。 |
| (b) 它能否跨 save/reload 持久、保有穩定身分？ | **不能可靠做到**。這是硬限制，理由見 §3。動態 form 的持久化由引擎 codec 決定：`TESNPC`（FormType::NPC）**不在會被序列化的型別清單裡**；即使是會被序列化的型別（WEAP/ARMO/MISC），重載後也**只剩 form type、資料全空**（無名、無模型）。SKSE 的 `SerializationInterface` 救不了「鑄造一個全新 base form」這件事（只能存 blob、remap 既有 plugin form 的 FormID）。 |
| 務實建議 | **co-save 食譜 + 讀檔重建** hybrid（§5），與 `PROCGEN_INTERIOR.md` §5.2 策略 B、`QUEST_ENGINE_DESIGN.md` 的 spawned 失效政策完全同一條路。或退一步：**複用/改寫既有 vanilla base**（§6）。 |

**一句話**：能造、能用一個 session；**不能讓「我發明的全新 base form」像 ESP 作者的 NPC 那樣自己活過存檔**。要持久，必須自己存食譜、讀檔重造。

---

本分析已按主題拆分：

- [既有地基、鑄造與持久化難題](PROCGEN_NPC_FORMS_MINTING_PERSISTENCE.md)：repo 既有地基、從零鑄造 `TESNPC_`、`CreateDuplicateForm`、動態 FormID 與 co-save。
- [外觀、hybrid、替代方案與裁決](PROCGEN_NPC_FORMS_APPEARANCE_OPTIONS.md)：外觀程序化、「生成 base＋暫時 ref」hybrid、替代方案、先例、直白裁決與務實建議。
- [最小 PoC、JSON adapter 與引用來源](PROCGEN_NPC_FORMS_IMPLEMENTATION.md)：最小 PoC 與單一未知數、`generate_npc` adapter 擴充動作、引用標頭。
