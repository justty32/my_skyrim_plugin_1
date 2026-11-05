# 原生 DialogueMenu spike：相關 RE 類別與真實簽名

[返回總索引](native_dialogue_spike.md)

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
