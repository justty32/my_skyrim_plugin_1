#pragma once

// NativeDialogueSpike (QUEST_ENGINE_DESIGN.md §5, research/native_dialogue_spike.md)
// — a Debug R&D probe for the "inject a custom topic into the player's NORMAL NPC
// dialogue" idea (the NativePresenter aspiration, DESIGN §3). It does NOT replace
// MessageBoxPresenter; the committed dialogue backend stays MessageBox.
//
// Staged, smallest-first (so each in-game test answers exactly one question):
//   Phase A — OBSERVATION (this build): on the native "Dialogue Menu" opening edge,
//     dump the live RE::MenuTopicManager state to the log (speaker, every Dialogue's
//     topicText, the list count). Proves our sink fires on real NPC conversations
//     and that we can READ dialogueList at that moment — the prerequisite for any
//     write/injection, and the cheapest way to disprove the whole approach.
//   Phase B — INJECTION: push a self-built Dialogue into dialogueList and see if GFx
//     renders it (the make-or-break; pinned by research/native_dialogue render path).
//   Phase C — SELECTION: detect which custom topic was picked and route it to a sink.
//
// Self-contained + cleanly removable: a singleton with Install()/Uninstall(),
// installed from OnDataLoaded() alongside the other adapter wiring.

namespace skyrim {

class NativeDialogueSpike {
public:
    static NativeDialogueSpike* GetSingleton();

    // Register the MenuOpenCloseEvent sink on RE::UI. Main thread, after kDataLoaded.
    void Install();
    void Uninstall();

private:
    NativeDialogueSpike() = default;
    class MenuSink;            // defined in the .cpp
    MenuSink* sink_ = nullptr;
};

}  // namespace skyrim
