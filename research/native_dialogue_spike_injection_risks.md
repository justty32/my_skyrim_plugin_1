# 原生 DialogueMenu spike：執行期注入與風險

[返回總索引](native_dialogue_spike.md)

## 3. 能不能執行期注入/強塞 topic？哪些可鑄 vs 需作者 form？

### 3.1 可鑄（dynamically mintable）

對照 `RE/Offsets_RTTI.h` 的 `ConcreteFormFactory_*` 清單與 `RE/Offsets_VTABLE.h`：

- `VTABLE_ConcreteFormFactory_TESTopic_75_`（`Offsets_VTABLE.h:947`）
- `VTABLE_ConcreteFormFactory_TESTopicInfo_76_`（`Offsets_VTABLE.h:949`）
- `ConcreteFormFactory_TESQuest`、`ConcreteFormFactory_BGSDialogueBranch` 亦在 RTTI 清單。

所以**整棵對話 form 樹都能用 `IFormFactory::GetConcreteFormFactoryByType<T>()->Create()` 鑄出來**
（`RE/I/IFormFactory.h:39-43`、`RE/C/ConcreteFormFactory.h:34-37`），與本 repo `NpcGenerator::SpawnNpc`
鑄 `TESNPC`、`InitializeMagic` 鑄 `EffectSetting`/`SpellItem`（見 `PROCGEN_NPC_FORMS.md` §0）**同一模式、已驗證可編譯/連結/session 內可用**。`DialogueResponse`/`DialogueItem` 是純記憶體結構，更可直接 `new`/ctor。

### 3.2 需作者 form 語意 / 引擎才認的部分（這才是真正的牆）

「鑄得出 form」只是第一步。要讓自製 topic **出現在選單**，引擎還需要：
- topic 掛在某個 `BGSDialogueBranch`（`TESTopic::ownerBranch`），branch 掛在 `TESQuest`，且 quest 必須
  is-running、branch 是 top-level（`MenuTopicManager::topLevelBranches`）；
- `TESTopicInfo::objConditions`（`TESCondition`）求值為真、`saidOnce`/`kSayOnce` 未擋、subtype 正確
  （`DIALOGUE_DATA::Subtype` 多達 100+ 種，玩家對話用 `kCustom`）；
- 引擎在「玩家 activate NPC」時跑的 **populate 函式**（未導出、需逆向）會掃這些 branch/topic，產生
  `MenuTopicManager::Dialogue` 並 push 給 GFx。執行期才鑄、沒走正常 load 流程接好的 form，**極可能被過濾掉**。

→ 結論：**dynamically mintable 的是 form 物件本身；不可繞過的是「引擎憑 branch/quest/condition 過濾、
再經未導出 populate 函式餵 GFx」這套作者預期的資料流。** 兩條候選路（與 `QUEST_ENGINE_DESIGN.md` §5 一致）：

1. **造 form 掛樹、靠引擎撈**：脆弱（過濾規則多、執行期接的樹未必被認）、且要正確設好 quest/branch/subtype。
2. **記憶體 + hook 注入**：`DialogueItem` ctor 建好 → 手塞 `MenuTopicManager::dialogueList` →
   trampoline hook 「topic 被選」callback → 驅動 `submitChoice()`。較可控，但要逆向 populate/select 位址，
   且 SE/AE/GOG/VR 位址各異（本 repo 的 `RELOCATION_ID(seID, aeID)` 雙 ID 模式只覆蓋已知偏移）。

### 3.3 save/reload 會壞什麼

- 動態鑄的 `TESTopic`/`TESTopicInfo`/`TESQuest` 得 `0xFF......` FormID，**不會被引擎的 form 序列化 codec
  正確存回**（與 `PROCGEN_NPC_FORMS.md` §3、`ALCHEMY_SPIKE_FINDINGS.md` 同一硬傷：dialogue 系 FormType
  不在會序列化的型別清單，且即使序列化也只剩空殼）。
- **但對話本質是 transient**：玩家不會在對話選單開著時存檔（`DialogueMenu` 期間通常擋存檔）。
  重載後我們**不需要**那些 form 還在——需要還在的是**我們自己的 engine progress**（哪個 quest、停在哪個
  dialogue node、`awaitingChoice`），這已由 `QuestEngine::exportProgress()/importProgress()` 處理
  （`src/core/QuestEngine.h:94-113`），重載後 `importProgress` 會「re-present 存檔當下的 node」。
- 真正的持久化風險只在：若 hook 在記憶體裡留了懸空指標（指向已被釋放的動態 form）。緩解：每次開對話
  「即用即鑄、關閉即棄」，絕不跨 save 持有。

### 3.4 語音 / lip / 字幕

- **無語音是已知硬傷**（`QUEST_ENGINE_DESIGN.md` §5 已記）。`DialogueResponse::voice`/`voiceSound` 空時，
  引擎正常流程傾向直接跳台詞、不給玩家足夠閱讀時間。
- `TOPIC_INFO_DATA::TOPIC_INFO_FLAGS`（`RE/T/TESTopicInfo.h:14-32`）有 `kForceSubtitle`(1<<9)、
  `kNoLIPFile`(1<<11)、`kSayOnce`(1<<2)：理論上 `kForceSubtitle|kNoLIPFile` 能「無語音強顯字幕、不要 lip」。
  但這些 flag 只在**走引擎 say 流程**（§2.5 的 `UpdateInDialogue`/`SetDialogueWithPlayer`）時生效。
- 走純記憶體/hook 路線則 flag 無從套用，字幕得自己 push `SubtitleManager::subtitles`（§2.6），
  時長也得自己管 timer——又是一堆手工活。
- 緩解備案（同 §5 筆記）：放一個極短的靜音 `.fuz` 當佔位 voice，讓引擎以為有語音、給足字幕時間。

---

## 4. 風險評估

| 面向 | MessageBox 保底 | 原生 DialogueMenu spike |
|---|---|---|
| 受支援 API | ✅ 全程 public（`MessageBoxData`/`IMessageBoxCallback`，已用於 `MessageBoxPresenter.cpp`） | ❌ 無「開自訂對話」入口；必走逆向 + hook |
| 跨遊戲版本（SE/AE/GOG/VR） | ✅ CommonLib 已封裝 | ❌ populate/select 位址需逐版本找；VR layout 還不同（`DialogueMenu` VR sizeof 不同，`RE/D/DialogueMenu.h:78-79`） |
| 持久化 | ✅ 不涉動態 form；engine blob 已處理 | ⚠️ 動態 form 不持久（但對話 transient，可規避）；懸空指標風險 |
| 語音/字幕 | ✅ 彈窗本身不需語音 | ❌ 無語音硬傷；字幕時長/lip 要手工 |
| 工程量 | 已完成 | 高（trampoline hook + 逆向 + 自管字幕/輸入回呼） |
| 回報 | 已滿足 go/no-go 準則 | 體驗更「原生」，但功能等價 |
| 崩潰風險 | 低 | 高（裸 member 注入、null 指標解參考、版本錯位） |

**致命點**：整套的成敗壓在「逆向出未導出的 populate/select 函式並穩定 hook」。這不是 header 能驗證的——
header 只證明**結構摸得到**，不證明**有受支援的入口**。本 repo 的雙 `RELOCATION_ID` 模式只覆蓋 CommonLib
已收錄的偏移；對話 populate 這條沒有現成 ID，得自己挖，且每次遊戲更新可能再壞。
