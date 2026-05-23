#include "skyrim/CoSave.h"

#include <vector>

#include <SKSE/API.h>
#include <SKSE/Interfaces.h>

namespace skyrim::cosave {

namespace {

// All registered module handlers. Populated by AddHandler (SKSEPluginLoad, before
// Register) and only iterated thereafter on the serialization thread, so no lock
// is needed: registration is single-threaded at load and the vector is never
// mutated again.
std::vector<Handler>& Handlers() {
    static std::vector<Handler> handlers;
    return handlers;
}

// The single SetSaveCallback: ask every handler to write its own record(s).
void OnSave(SKSE::SerializationInterface* intfc) {
    for (const auto& h : Handlers()) {
        if (h.save) h.save(intfc);
    }
    SKSE::log::info("CoSave: OnSave fanned out to {} handler(s)", Handlers().size());
}

// The single SetLoadCallback: walk the co-save records and dispatch each by its
// 4-char type to the owning handler (the same shape as SKSE dispatching plugins
// by unique id). The handler reads ONLY its own record's payload; we feed it the
// record's (version, length) so it can version-gate without re-reading the header.
void OnLoad(SKSE::SerializationInterface* intfc) {
    std::uint32_t type = 0, version = 0, length = 0;
    std::uint32_t dispatched = 0;
    while (intfc->GetNextRecordInfo(type, version, length)) {
        const Handler* match = nullptr;
        for (const auto& h : Handlers()) {
            if (h.recordType == type && h.load) {
                match = &h;
                break;
            }
        }
        if (match) {
            match->load(intfc, version, length);
            ++dispatched;
        } else {
            SKSE::log::warn("CoSave: OnLoad no handler for record '{:08X}' (skipped)", type);
        }
    }
    SKSE::log::info("CoSave: OnLoad dispatched {} record(s)", dispatched);
}

// The single SetRevertCallback: fan out to every handler to clear its registry.
void OnRevert(SKSE::SerializationInterface* intfc) {
    for (const auto& h : Handlers()) {
        if (h.revert) h.revert(intfc);
    }
    SKSE::log::info("CoSave: OnRevert fanned out to {} handler(s)", Handlers().size());
}

}  // namespace

void AddHandler(const Handler& handler) {
    if (handler.recordType == 0) {
        SKSE::log::error("CoSave: AddHandler with zero recordType (ignored)");
        return;
    }
    for (const auto& h : Handlers()) {
        if (h.recordType == handler.recordType) {
            SKSE::log::error("CoSave: duplicate handler for record '{:08X}' (ignored)",
                             handler.recordType);
            return;
        }
    }
    Handlers().push_back(handler);
    SKSE::log::info("CoSave: registered handler for record '{:08X}'", handler.recordType);
}

bool Register(const SKSE::SerializationInterface* intfc) {
    if (!intfc) {
        SKSE::log::error("CoSave: Register got a null SerializationInterface");
        return false;
    }
    // The plugin's ONE registration. Doing this once here is the whole point: no
    // module may call SetUniqueID/SetSaveCallback itself or it clobbers the rest.
    intfc->SetUniqueID(kPluginUniqueID);
    intfc->SetSaveCallback(OnSave);
    intfc->SetLoadCallback(OnLoad);
    intfc->SetRevertCallback(OnRevert);
    SKSE::log::info("CoSave: registered central dispatcher (uid '{:08X}', {} handler(s))",
                    kPluginUniqueID, Handlers().size());
    return true;
}

}  // namespace skyrim::cosave
