#pragma once

// MessageBoxPresenter (DESIGN §3) — the low-risk, "almost no risk" fallback
// DialoguePresenter that drives the whole quest flow before any native-dialogue
// spike (DESIGN §5). Implements qe::IDialoguePresenter.
//
// Resumable contract (Ports.h): presentNode is DISPLAY-ONLY and MUST NOT block.
//   - choice node  -> queue a RE::MessageBoxData with one button per choice; its
//                     async callback fires onChoice(idx) when the player clicks.
//   - terminal node (choices empty) -> show lines as an OK message box.
//   - showMessage  -> screen notification (RE::DebugNotification).
// The adapter wires onChoice() to QuestEngine::submitChoice() (idx<0 == cancel).
//
// All UI calls are queued onto the main thread by the adapter before reaching
// here, and the MessageBox callback itself runs on the UI/main thread.

#include <functional>
#include <string>
#include <vector>

#include "core/Ports.h"

namespace skyrim {

class MessageBoxPresenter : public qe::IDialoguePresenter {
public:
    // Set by the adapter: routes the player's pick back into the engine.
    // idx is 0-based into the last presented choices; idx < 0 means cancel.
    void setChoiceSink(std::function<void(int)> sink) { onChoice_ = std::move(sink); }

    void presentNode(const std::string& speaker,
                     const std::vector<std::string>& lines,
                     const std::vector<std::string>& choices) override;
    void showMessage(const std::string& text) override;

private:
    std::function<void(int)> onChoice_;
};

}  // namespace skyrim
