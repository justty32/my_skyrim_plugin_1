#include "NativeDialogueSpike.h"

#include <string>

namespace skyrim {

// Phase A — OBSERVATION sink. Read-only: when the native "Dialogue Menu" opens,
// walk RE::MenuTopicManager and log the live state. This is deliberately a pure
// READ (no push into dialogueList) so it cannot crash — it answers the questions
// that gate any later injection: does our sink fire on a real NPC conversation?
// is dialogueList non-null at the open edge? can we read each Dialogue's topicText?
// (Both background research agents flagged that WRITING — push + the engine freeing
//  our node on close + a click dereferencing null parents — is where the crashes
//  live, and that whether GFx even re-renders a pushed entry is a coin-flip only an
//  in-game test settles. So we read first, write later, eyes open.)
class NativeDialogueSpike::MenuSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
        if (!a_event) return RE::BSEventNotifyControl::kContinue;
        // Only the Dialogue Menu, only the opening edge.
        if (a_event->menuName != RE::DialogueMenu::MENU_NAME || !a_event->opening) {
            return RE::BSEventNotifyControl::kContinue;
        }
        DumpTopicManager();
        return RE::BSEventNotifyControl::kContinue;
    }

private:
    static void DumpTopicManager() {
        auto* mtm = RE::MenuTopicManager::GetSingleton();
        if (!mtm) {
            SKSE::log::warn("NativeDialogueSpike: Dialogue Menu opened but MenuTopicManager is null");
            return;
        }
        RE::NiPointer<RE::TESObjectREFR> speaker = mtm->speaker.get();
        SKSE::log::info("NativeDialogueSpike: Dialogue Menu OPENED  speaker={:08X}  greeting={}  "
                        "dialogueList={}", speaker ? speaker->GetFormID() : 0u,
                        mtm->isGreetingPlayer,
                        static_cast<const void*>(mtm->dialogueList));
        if (!mtm->dialogueList) {
            SKSE::log::info("NativeDialogueSpike:   dialogueList is NULL at the open edge "
                            "(populate likely runs later — injection timing matters)");
            return;
        }
        int n = 0;
        RE::MenuTopicManager::Dialogue* firstTopic = nullptr;
        for (auto* d : *mtm->dialogueList) {
            if (d) {
                if (!firstTopic) firstTopic = d;
                SKSE::log::info("NativeDialogueSpike:   topic[{}] text='{}'  responses_empty={}",
                                n, d->topicText.c_str(), d->responses.empty());
            } else {
                SKSE::log::info("NativeDialogueSpike:   topic[{}] <null Dialogue*>", n);
            }
            ++n;
        }
        SKSE::log::info("NativeDialogueSpike:   live topic count at open = {}", n);

        // >>> native-inject (Phase B-2): FORM-edit probe. Phase B-1 proved a live
        // dialogueList edit RENDERS but the engine re-populates from the form a few
        // seconds later and REVERTS it. So this probe edits BOTH: the transient list
        // entry (immediate display) AND the underlying TESTopic form's fullName (the
        // FULL field the menu line comes from). If the form now holds our text, the
        // periodic re-populate reads OUR text and should NOT revert. This is the
        // runtime-in-memory version of "modify the dialogue content directly".
        // NOTE: editing the form is SESSION-WIDE (every NPC using this topic shows
        // the marked text until the game restarts) and does NOT persist to saves.
        if (firstTopic) {
            const std::string original = firstTopic->topicText.c_str();
            const bool alreadyMarked = original.rfind("[FORM] ", 0) == 0;
            if (!alreadyMarked) {
                const std::string marked = "[FORM] " + original;
                auto* topicForm = firstTopic->parentTopic;  // the TESTopic behind this row
                REX::W32::EnterCriticalSection(&mtm->criticalSection);
                firstTopic->topicText = marked.c_str();              // transient list (immediate)
                if (topicForm) topicForm->fullName = marked.c_str(); // FORM (survives re-populate)
                REX::W32::LeaveCriticalSection(&mtm->criticalSection);
                SKSE::log::info("NativeDialogueSpike:   FORM PROBE topic[0] '{}' -> '{}'  "
                                "parentTopic={:08X}  (in-game: does it now STAY marked, "
                                "no revert after a few seconds?)",
                                original, marked,
                                topicForm ? topicForm->GetFormID() : 0u);
            } else {
                SKSE::log::info("NativeDialogueSpike:   FORM PROBE topic[0] already marked "
                                "('{}') — form edit from a prior open persisted", original);
            }
        } else {
            SKSE::log::info("NativeDialogueSpike:   FORM PROBE skipped (no topic)");
        }
        // <<< native-inject
    }
};

NativeDialogueSpike* NativeDialogueSpike::GetSingleton() {
    static NativeDialogueSpike instance;
    return &instance;
}

void NativeDialogueSpike::Install() {
    auto* ui = RE::UI::GetSingleton();
    if (!ui) {
        SKSE::log::error("NativeDialogueSpike: no RE::UI — cannot install MenuOpenCloseEvent sink");
        return;
    }
    if (!sink_) {
        sink_ = new MenuSink();
        ui->AddEventSink<RE::MenuOpenCloseEvent>(sink_);
        SKSE::log::info("NativeDialogueSpike: MenuOpenCloseEvent sink installed (Phase A observation)");
    }
}

void NativeDialogueSpike::Uninstall() {
    auto* ui = RE::UI::GetSingleton();
    if (ui && sink_) ui->RemoveEventSink<RE::MenuOpenCloseEvent>(sink_);
    delete sink_;
    sink_ = nullptr;
}

}  // namespace skyrim
