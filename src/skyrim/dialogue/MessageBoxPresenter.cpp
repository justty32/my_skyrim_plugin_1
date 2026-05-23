#include "MessageBoxPresenter.h"

namespace skyrim {

namespace {

// One-shot IMessageBoxCallback: the engine invokes Run(Message) with the chosen
// button index (the Message enum value == 0-based button index for a multi-
// button MessageBoxData). We forward that index to the engine via the sink.
//
// Ref-counting: created with RE::make_smart and handed to MessageBoxData.callback
// (a BSTSmartPointer<IMessageBoxCallback>); the menu releases it after Run.
class ChoiceCallback : public RE::IMessageBoxCallback {
public:
    explicit ChoiceCallback(std::function<void(int)> sink, int cancelIdx)
        : sink_(std::move(sink)), cancelIdx_(cancelIdx) {}

    void Run(Message a_msg) override {
        const int idx = static_cast<int>(a_msg);
        // The dedicated cancel button (if any) maps to "cancel" (idx < 0).
        const int picked = (cancelIdx_ >= 0 && idx == cancelIdx_) ? -1 : idx;
        if (sink_) sink_(picked);
    }

private:
    std::function<void(int)> sink_;
    int cancelIdx_;
};

void QueueMessageBox(const std::string& body, const std::vector<std::string>& buttons,
                     std::function<void(int)> sink) {
    auto* factory = RE::MessageDataFactoryManager::GetSingleton();
    auto* strings = RE::InterfaceStrings::GetSingleton();
    if (!factory || !strings) {
        SKSE::log::error("MessageBoxPresenter: cannot get MessageBox factory/strings");
        if (sink) sink(-1);
        return;
    }
    auto* creator = factory->GetCreator<RE::MessageBoxData>(strings->messageBoxData);
    auto* mbox = creator ? creator->Create() : nullptr;
    if (!mbox) {
        SKSE::log::error("MessageBoxPresenter: failed to create MessageBoxData");
        if (sink) sink(-1);
        return;
    }

    mbox->bodyText = body.c_str();
    for (const auto& b : buttons) mbox->buttonText.push_back(RE::BSString(b.c_str()));
    mbox->isCancellable = false;
    mbox->cancelOptionIndex = -1;

    // Hand a fresh callback to the menu (it owns it via the smart pointer).
    mbox->callback = RE::make_smart<ChoiceCallback>(std::move(sink), -1);
    mbox->QueueMessage();
}

}  // namespace

void MessageBoxPresenter::presentNode(const std::string& speaker,
                                      const std::vector<std::string>& lines,
                                      const std::vector<std::string>& choices) {
    // Build the body: "speaker: line" joined by newlines.
    std::string body;
    for (const auto& l : lines) {
        if (!body.empty()) body += "\n";
        if (!speaker.empty()) body += speaker + "：";
        body += l;
    }

    if (choices.empty()) {
        // Terminal node: an OK message box (no engine resume expected).
        std::vector<std::string> ok{"OK"};
        QueueMessageBox(body, ok, nullptr);
        return;
    }

    // Choice node: one button per choice; async callback resumes the engine.
    auto sink = onChoice_;  // copy: callback outlives this call
    QueueMessageBox(body, choices, [sink](int idx) {
        if (sink) sink(idx);
    });
}

void MessageBoxPresenter::showMessage(const std::string& text) {
    // SPEC §4.2 show_message: immediate, one-way HUD notification.
    RE::DebugNotification(text.c_str());
}

}  // namespace skyrim
