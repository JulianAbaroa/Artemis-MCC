export module DevTools.Workbench.TickDump.Type;

import Blam.Layout.Snapshot.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Workbench::TickDump::Type
{
    // One tick waiting to be written: the Tick for the .tdump and the Snapshot it was built from for the .sdump (D206).
    struct TickDumpEntry
    {
        std::shared_ptr<const Bridge::Codec::Tick::Type::Tick> Tick{};
        std::shared_ptr<const Blam::Layout::Snapshot::Type::Snapshot> Snapshot{};
        std::uint64_t Generation{};
        std::uint32_t DroppedBefore{};
        // The first tick that ran after a map load.
        bool IsFirstAfterLoad{};
        // The viewer camera was active in the tick.
        bool IsViewerActive{};
    };
}