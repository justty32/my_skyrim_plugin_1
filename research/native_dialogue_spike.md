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

## 2. 相關 RE 類別與真實簽名（逐一查證）

### 2.1 `RE::DialogueMenu` — `RE/D/DialogueMenu.h`

是個 Scaleform GFx menu（繼承 `IMenu`）：

```cpp
// RE/D/DialogueMenu.h:14-25
class DialogueMenu :
    public IMenu,                            // 00
    public BSTEventSink<MenuOpenCloseEvent>  // 30
{
    constexpr static std::string_view MENU_NAME = "Dialogue Menu";
    // override (IMenu)
    void               Accept(CallbackProcessor* a_processor) override;  // 01
    UI_MESSAGE_RESULTS ProcessMessage(UIMessage& a_message) override;    // 04
    // RUNTIME_DATA: BSTArray<Data> unk38;  (Data = {void* unk00; uint64 unk08})
};
```

關鍵：**它沒有任何「加一個 topic 選項」的 public 方法**。`Data`（`{void* unk00; uint64 unk08}`，
`RE/D/DialogueMenu.h:27-32`）是匿名的 runtime 陣列，內容語意未公開。選單的可見列是由 GFx 影片
（`IMenu` 繼承 `FxDelegateHandler`，`RE/I/IMenu.h:54`；持有 `GFxMovieView`，`RE/I/IMenu.h:5`）
經 `FxDelegate` callback 從 C++ 端 push 進去的——這是 vanilla 內部流程，**沒有導出**。

### 2.2 `RE::MenuTopicManager` — `RE/M/MenuTopicManager.h`（核心）

這是真正持有「目前這段對話有哪些可選 topic」的 singleton：

```cpp
// RE/M/MenuTopicManager.h:61-65
static MenuTopicManager* GetSingleton()
{
    REL::Relocation<MenuTopicManager**> singleton{ RELOCATION_ID(514959, 401099) };
    return *singleton;
}

// 內部 Dialogue 結構（一個可選 topic + 它的 responses），RE/M/MenuTopicManager.h:30-51
struct Dialogue {
    BSString                          topicText;        // 00  ← 選單上顯示的那行 topic 文字
    BSSimpleList<DialogueResponse*>   responses;        // 18  ← 點下去 NPC 講的台詞們
    TESQuest*                         parentQuest;      // 28
    TESTopicInfo*                     parentTopicInfo;  // 30
    TESTopic*                         parentTopic;      // 38
    bool                              neverSaid;        // 49
    // ...
};

// members（RE/M/MenuTopicManager.h:67-93）
BSSimpleList<Dialogue*>::Node* selectedResponseNode;  // 18
BSSimpleList<Dialogue*>*       dialogueList;          // 20  ← 整份可選 topic 清單
TESTopicInfo*                  rootTopicInfo;         // 30
ObjectRefHandle                speaker;               // 68  ← 對話對象
TESTopicInfo*                  currentTopicInfo;      // 70 - only valid when NPC talking
BSTArray<BGSDialogueBranch*>   topLevelBranches;      // 98
bool                           isGreetingPlayer;      // B0
```

**這是注入路線的著力點**：`dialogueList` 是 `BSSimpleList<Dialogue*>*`，理論上可手動塞 `Dialogue*`。
`topicText` 就是選單顯示文字，`responses` 是點選後的台詞。**但**：
- 沒有任何 public 方法去「新增一個 Dialogue 並通知 GFx 重繪」。`dialogueList` 是裸 member。
- `Dialogue` 內有 `parentTopic`/`parentTopicInfo`/`parentQuest` 指標，玩家點選後引擎會沿著它們跑
  （標記 `saidOnce`、跑 `objConditions`、推進 branch）——若塞假的/null 指標，行為未定義。
- 重繪、選取回呼、語音播放都在 vanilla 的 GFx populate / `ProcessMessage` 路徑裡，**要 hook 才接得到**。

### 2.3 `RE::TESTopic` / `RE::TESTopicInfo` — 是真 form，可鑄但需作者語意

```cpp
// RE/T/TESTopic.h:132-176  — FormType::Dialogue
class TESTopic : public TESForm, public TESFullName {
    DIALOGUE_DATA      data;                     // 30 (topicFlags/type/subtype)
    BGSDialogueBranch* ownerBranch;              // 38 - BNAM
    TESQuest*          ownerQuest;               // 40 - QNAM
    TESTopicInfo**     topicInfos;               // 48 - infoTopics[infoCount]
    std::uint32_t      numTopicInfos;            // 50 - TIFC
};
```

```cpp
// RE/T/TESTopicInfo.h:41-145  — FormType::Info
class TESTopicInfo : public TESForm {
    TESTopic*     parentTopic;    // 20
    TESCondition  objConditions;  // 30 - CTDA  ← 引擎用來決定「這條 info 現在能不能講」
    bool          saidOnce;       // 3A
    TOPIC_INFO_DATA data;         // 3C - ENAM (flags: kForceSubtitle/kNoLIPFile/kSayOnce/...)
    // 內含 ResponseData（TRDT）鏈：emotionType / responseText(NAM1) / sound / speakerIdle ...
    DialogueItem GetDialogueData(Actor* a_speaker);  // RE/T/TESTopicInfo.cpp:30
};
```

`GetDialogueData` 實作（`RE/T/TESTopicInfo.cpp:30-33`）就是
`{ parentTopic->ownerQuest, parentTopic, this, a_speaker }` → 一個 `DialogueItem`。
**注意**：它直接解參考 `parentTopic->ownerQuest`，所以鑄一個孤兒 `TESTopicInfo`（沒接 quest/topic）
呼叫它會崩。要走引擎流程，得把 quest→branch→topic→info 整棵接好。

### 2.4 `RE::DialogueItem` / `RE::DialogueResponse` — `RE/D/DialogueItem.h`（記憶體層、有 public ctor）

```cpp
// RE/D/DialogueItem.h:40-69  — 有 public 建構子，且 Ctor 走 RELOCATION_ID(34413, 35220)
class DialogueItem : public BSIntrusiveRefCounted {
    DialogueItem(TESQuest* a_quest, TESTopic* a_topic, TESTopicInfo* a_topicInfo, Actor* a_speaker);
    BSSimpleList<DialogueResponse*>        responses;   // 08
    TESTopicInfo*                          info;        // 20
    TESTopic*                              topic;       // 28
    TESQuest*                              quest;       // 30
    TESObjectREFR*                         speaker;     // 38
};

// RE/D/DialogueItem.h:21-37  — 選單實際 render 的單句結構
class DialogueResponse {
    BSString       text;        // 00  ← 字幕文字
    std::uint16_t  percent;     // 14
    BSFixedString  voice;       // 18  ← 語音檔名（無檔 → 引擎傾向跳過）
    bool           soundLip;    // 39
};
```

`DialogueItem` 的 public ctor 仍呼叫 vanilla 函式（`RELOCATION_ID(34413, 35220)`），它**期望**傳入
真的 quest/topic/info。所以「先鑄 form 再建 DialogueItem」這條，依然繞不開「要有一棵接好的 form 樹」。

### 2.5 觸發進入對話：`Actor` / `TESObjectREFR` 上的方法

- `Actor::SetDialogueWithPlayer(bool a_flag, bool a_forceGreet, TESTopicInfo* a_topic)`（vtable 0x41，
  `RE/A/Actor.h:288`）— **需要一個 `TESTopicInfo*`**。傳 vanilla 的能用；傳自鑄孤兒的會走進 §2.3 的崩潰。
- `Actor::InitiateDialogue(Actor* a_target, PackageLocation*, PackageLocation*)`（vtable 0xD8，
  `RE/A/Actor.h:414`）/ `Actor::EndDialogue()`（0xDA，`RE/A/Actor.h:416`）— 啟動/結束對話流程。
- `TESObjectREFR::UpdateInDialogue(DialogueResponse*, bool)`（vtable 0x4C，`RE/T/TESObjectREFR.h:272`；
  `Actor` 覆寫於 `RE/A/Actor.h:294`）— 推進一句 response（播語音/字幕/lip）。
- `Actor` flags（`RE/A/Actor.h`）：`kForceGreetingPlayer`(1<<12, :171)、`kVoiceFileDone`(1<<8, :167)、
  `kDoNotRunSayToCallback`(1<<10, :169)。

**沒有**任何 `Say(topicText, ...)` 或 `ShowDialogueMenu(customTopics)` 之類「給文字就開對話」的入口
（grep 整棵 `RE/` 找不到 `ShowDialogueMenu`/`SayTopic`/`StartConversation` 的可呼叫宣告，只在
`Offsets_*`/`TESCondition` 裡有不相關命中）。

### 2.6 字幕：`RE::SubtitleManager` — `RE/S/SubtitleManager.h`（純 hook 路線的救生圈）

```cpp
// RE/S/SubtitleManager.h:21-44
class SubtitleManager : public BSTSingletonSDM<SubtitleManager> {
    static SubtitleManager* GetSingleton();          // RELOCATION_ID(514283, 400443)
    void KillSubtitles();                             // RELOCATION_ID(51755, 52628)
    BSTArray<SubtitleInfo> subtitles;                 // 18
    ObjectRefHandle        currentSpeaker;            // 28
};
// SubtitleInfo { ObjectRefHandle speaker; BSString subtitle; float targetDistance; bool forceDisplay; }
```

若走「不靠引擎 say、純記憶體驅動」路線，字幕得自己 push `SubtitleInfo`（`subtitles` 是裸 `BSTArray`，
無 public add 方法 → 又是手動操作）。`forceDisplay` 對應字幕強顯。

### 2.7 `RE::MenuControls` / `RE::Console` — 不相關

`RE/M/MenuControls.h`（輸入路由）、`RE/C/Console.h` 對「驅動對話內容」無直接幫助；Console 只能跑
console 指令字串，沒有開自訂對話的指令。排除。

---

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

---

## 5. VERDICT 與最小原生實驗

**VERDICT：維持 `MessageBoxPresenter` 為 production 保底，不上原生 DialogueMenu。**

理由精確版：
1. **沒有受支援的執行期入口**。grep 整棵 `RE/` 沒有「給文字 + 選項就開對話」的可呼叫 API；
   `DialogueMenu`/`MenuTopicManager` 只導出 layout，不導出 populate。
2. **唯一可行路徑是逆向 + hook + 裸記憶體注入**，跨 SE/AE/GOG/VR 位址各異、易隨更新崩，風險/回報不成比例。
3. **MessageBox 已滿足既定 go/no-go 準則**（`QUEST_ENGINE_DESIGN.md` §5:137：自訂文字 + ≥2 可點選項 +
   點選回呼 C++）。`MessageBoxPresenter.cpp` 已實證做到（多按鈕 + `ChoiceCallback::Run` 回 `submitChoice`）。
4. **無語音硬傷**讓原生體驗其實也不完整（NPC 不出聲、嘴不動），原生的唯一賣點（沉浸感）被自己打折。

**若仍想取得可行性訊號，最小原生實驗（go/no-go 一次性 spike，不接 production）：**
- 目標（沿用 §5 準則）：對一個**任意 vanilla NPC**，在玩家 activate 時，於選單**多出一行自訂 topic 文字**，
  點下去顯示一句自訂字幕，並讓 C++ 收到「被選」事件。
- 最小步驟：
  1. Hook `DialogueMenu::ProcessMessage`（vtable 0x04，`RE/D/DialogueMenu.h:44`）或開選單事件，
     在選單開啟瞬間，往 `MenuTopicManager::GetSingleton()->dialogueList`（`RE/M/MenuTopicManager.h:69`）
     塞一個自建 `Dialogue`（`topicText` = 我們的選項字，`responses` = 一句 `DialogueResponse{text=...}`）。
  2. 驗證 GFx 是否會把新 `Dialogue` 畫出來（**這步最可能失敗**——若 populate 已跑完、不重掃 list，就畫不出）。
     若畫不出，改 hook 引擎的 populate 函式本體（需先逆向其位址）。
  3. Hook topic-selected callback，確認點選後能拿到對應 `Dialogue*` → 呼叫我們的 sink。
  4. 用 `kForceSubtitle` + 靜音 `.fuz` 驗證字幕能停留夠久。
- **判定**：步驟 2 在不 hook populate 本體的前提下成功 → 有戲，可評估 production；
  需要 hook populate 本體才行 → **no-go**，停在 MessageBox（成本/版本脆弱性不值得）。
- 此實驗應落在獨立的 `src/skyrim/dialogue/NativePresenter.{h,cpp}`（`QUEST_ENGINE_DESIGN.md` §6:161 已預留位置），
  且只在 Debug 建置、用一個 feature flag 隔離，絕不取代保底。

---

## 6. 如何接進 resumable presenter（無論走哪條都這樣接）

好消息：**core 完全不用改**。`NativePresenter` 只要實作既有的 `qe::IDialoguePresenter`
（`src/core/Ports.h:20-27`）三件事，就能跟 `MessageBoxPresenter` 互換：

```cpp
// src/core/Ports.h:20-27（既有契約，display-only、不阻塞）
class IDialoguePresenter {
    virtual void presentNode(const std::string& speaker,
                             const std::vector<std::string>& lines,
                             const std::vector<std::string>& choices) = 0;
    virtual void showMessage(const std::string& text) = 0;
};
```

接線對照（與 `MessageBoxPresenter` 完全平行）：

1. **`presentNode`**：
   - `choices` 非空（choice node）→ 開原生對話／注入 N 個 topic 列（每個 `choices[i]` 一行）、
     上方顯示 `speaker` + `lines`；保留 `onChoice_` sink（`MessageBoxPresenter.h:29` 同款
     `setChoiceSink`）供回呼。
   - `choices` 為空（terminal node）→ 顯示 `lines` 後直接結束對話（不等回呼），對應
     `MessageBoxPresenter::presentNode` 的 OK 分支（`MessageBoxPresenter.cpp:70-74`）。
2. **點選回呼**：原生 topic-selected hook 拿到 index → 呼叫 `onChoice_(idx)`（cancel → idx<0），
   與 `ChoiceCallback::Run`（`MessageBoxPresenter.cpp:18-23`）職責相同。
3. **adapter 接回引擎**：`SkyrimAdapter::BuildEngine` 把 sink 接到 `SubmitChoice`
   （`src/skyrim/SkyrimAdapter.cpp:121`：`presenter_.setChoiceSink([](int idx){ ...SubmitChoice(idx); })`），
   `SubmitChoice` 在 main thread 上呼叫 `engine_->submitChoice(idx)`（`SkyrimAdapter.cpp:218-228`），
   並用 `awaitingChoice()` 守門。**換 presenter 只需把 `SkyrimAdapter::presenter_` 的型別換成
   `NativePresenter`（或用一個 `IDialoguePresenter*` 指標 + flag 選），sink 接線一字不改。**
4. **主執行緒 / 重繪**：所有 UI 呼叫已由 adapter 的 `OnMainThread` marshalling 保證在 main/UI thread
   （`MessageBoxPresenter.h:14-15` 註明）；原生注入（碰 `MenuTopicManager`/GFx）**同樣必須**在 UI thread，
   沿用既有 marshalling 即可。
5. **存讀檔**：`importProgress` 重載後會 re-present 存檔當下的 node（`QuestEngine.h:106-113`），
   `NativePresenter` 只要 `presentNode` 是冪等的顯示動作即可——**不需要**自己持久化任何 form。

→ 因此即使日後原生 spike 成功，**改動面極小**：新增一個 `NativePresenter` 檔、在 `SkyrimAdapter` 換指標，
core/Ports/QuestEngine/submitChoice 全部不動。這也反證「先用 MessageBox 打通、之後再可選升級」的分層是對的。

---

## 7. 待逆向 / 未由 header 證實的缺口（給真要做 spike 的人）

header **無法**回答、必須靠逆向/實機驗證的點：
- 引擎在「玩家 activate NPC」時跑的 **dialogue populate 函式位址**（產生 `MenuTopicManager::Dialogue`
  並餵 GFx 的那個）—— 沒有現成 `RELOCATION_ID`。
- **topic-selected callback** 的攔截點（玩家點某行 topic 時引擎走哪、能不能改派到我們的 sink）。
- 手塞 `dialogueList` 後 GFx **會不會重繪**，還是 populate 一次後就快取。
- `Dialogue` 的 `unk10`~`unk4C` 等未命名欄位在「自建 Dialogue」時是否必須正確初始化才不崩。
- 無語音時 `kForceSubtitle` 在執行期鑄的 info 上**實際**字幕停留時長。

這些都是 R&D，不是查 header 能定案——也正是 VERDICT 給「no-go / 維持 MessageBox」的根據。
