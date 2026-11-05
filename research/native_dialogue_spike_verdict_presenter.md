# 原生 DialogueMenu spike：VERDICT、presenter 接法與缺口

[返回總索引](native_dialogue_spike.md)

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
