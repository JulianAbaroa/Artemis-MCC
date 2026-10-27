export module UI.TickDump.Type;

import Layout.Snapshot.Type;
import Export.Tick.Type;
import std;

export namespace UI::TickDump::Type
{
    // One tick waiting to be written: the Tick for the .tdump and the Snapshot it was built from for the .sdump (D206).
    struct TickDumpEntry
    {
        std::shared_ptr<const Export::Tick::Type::Tick> Tick{};
        std::shared_ptr<const Layout::Snapshot::Type::Snapshot> Snapshot{};
        std::uint64_t Generation{};
        std::uint32_t DroppedBefore{};
        // The first tick that ran after a map load.
        bool IsFirstAfterLoad{};
        // The viewer camera was active in the tick.
        bool IsViewerActive{};
    };
}