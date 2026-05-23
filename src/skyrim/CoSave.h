#pragma once

// CoSave — the plugin's ONE central SKSE co-save (SerializationInterface)
// dispatcher (research/PROCGEN_INTERIOR.md §5.3 / PROCGEN_NPC_FORMS.md §5).
//
// WHY THIS EXISTS (the hard SKSE limit): SerializationInterface allows only ONE
// SetUniqueID + ONE each of SetSaveCallback / SetLoadCallback / SetRevertCallback
// PER PLUGIN — the last registration wins and SILENTLY clobbers any earlier one.
// gen-npc ('GNPC') and procgen ('PRGN') both need co-save persistence, so they
// must NOT each call SetUniqueID/SetSaveCallback. Instead this module owns the
// single registration and fans the three callbacks out to per-module handlers
// keyed by a 4-char record type, exactly the way SKSE itself dispatches plugins
// by their unique id.
//
// HOW MODULES USE IT: each module calls AddHandler({recordType, saveFn, loadFn,
// revertFn}) ONCE at static-init / SKSEPluginLoad time (before Register runs).
//   - saveFn:  OpenRecord('TYPE', ver) + WriteRecordData(...) for that module.
//   - loadFn:  invoked with the record's (version, length) AFTER the dispatcher
//              has already consumed the type via GetNextRecordInfo; the handler
//              reads its own payload (and ResolveFormID-remaps plugin FormIDs).
//   - revertFn: clear that module's in-memory registry.
// The save callback iterates every handler; the load callback walks the co-save
// records with GetNextRecordInfo and dispatches each to the owning handler by
// type; revert fans out to all handlers.
//
// THREADING: SKSE invokes Save/Load/Revert on its serialization thread, NOT the
// main thread. Handlers must therefore NOT touch RE:: live state directly — they
// stage data and defer RE:: work to the next kPostLoadGame main-thread tick (see
// each module's RebuildStaged(); research §5 "時機" / MODDING_COOKBOOK R9).

#include <cstdint>

namespace SKSE {
class SerializationInterface;
}

namespace skyrim::cosave {

// The plugin's single SerializationInterface unique id ('TPL1' = Template Plugin
// co-save v1). This is the ONE id passed to SetUniqueID — distinct from any
// per-module record type below. Bump the trailing digit only if the whole
// co-save layout is broken in an incompatible way.
inline constexpr std::uint32_t kPluginUniqueID = 'TPL1';

// A handler is invoked by the save/load callbacks. recordType is the 4-char id
// the module writes via OpenRecord and the dispatcher matches on load.
struct Handler {
    std::uint32_t recordType = 0;  // e.g. 'GNPC', 'PRGN'

    // Write this module's record(s). Called once per save on the serialization
    // thread. Typically: intfc->OpenRecord(recordType, version) + WriteRecordData.
    void (*save)(SKSE::SerializationInterface* intfc) = nullptr;

    // Read this module's payload for ONE record the dispatcher already matched by
    // type via GetNextRecordInfo. version/length are that record's header values.
    // Called on the serialization thread; stage, do not touch RE:: live state.
    void (*load)(SKSE::SerializationInterface* intfc, std::uint32_t version,
                 std::uint32_t length) = nullptr;

    // Drop this module's in-memory registry (engine tears down the outgoing save).
    void (*revert)(SKSE::SerializationInterface* intfc) = nullptr;
};

// Register a module's handler. Call ONCE per module before Register() runs (i.e.
// from SKSEPluginLoad, ahead of cosave::Register). Ignored (logged) if a handler
// with the same recordType is already present.
void AddHandler(const Handler& handler);

// Install the plugin's single SetUniqueID + the three central callbacks on the
// SerializationInterface. Call ONCE from SKSEPluginLoad after SKSE::Init and
// after all AddHandler calls. Returns false if the interface is null.
bool Register(const SKSE::SerializationInterface* intfc);

}  // namespace skyrim::cosave
