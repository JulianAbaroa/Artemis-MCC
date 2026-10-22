export module UI.TickDump.Type;

import Export.Tick.Type;
import std;

export namespace UI::TickDump::Type
{
    // Version of the .tdump format (D151). Raised when the file header, the record header or the order of the blocks changes.
    inline constexpr std::uint32_t k_TickDumpVersion{ 1 };

    // First bytes of a .tdump file.
    inline constexpr std::array<char, 8> k_TickDumpMagic{ 'A', 'T', 'D', 'U', 'M', 'P', '\0', '\0' };

    // Marker of each tick record.
    inline constexpr std::array<char, 4> k_TickRecordMarker{ 'T', 'I', 'C', 'K' };

    // Marker of the footer, written when the recording stops.
    inline constexpr std::array<char, 4> k_TickDumpEndMarker{ 'T', 'E', 'N', 'D' };

    // TickRecordHeader::Flags: the Tick had a Match (GameTick is valid).
    inline constexpr std::uint16_t k_TickRecordHasMatch{ 1 };

    // TickRecordHeader::Flags: events left the window of the BridgeTick before they were written (some are missing).
    inline constexpr std::uint16_t k_TickRecordEventsLost{ 2 };

    // TickRecordHeader::Flags: the record carries every live entity, not the changes since the previous record.
    inline constexpr std::uint16_t k_TickRecordKeyframe{ 4 };

    // Start of a .tdump file, followed by LayoutJsonSize bytes of k_BridgeLayoutJson.
    struct TickDumpFileHeader
    {
        std::array<char, 8> Magic{};
        std::uint32_t Version{};
        // Size of this header, so a reader can skip fields it does not know.
        std::uint32_t HeaderSize{};
        std::uint64_t LayoutHash{};
        std::uint32_t LayoutVersion{};
        std::uint32_t LayoutJsonSize{};
        std::uint32_t RecordHeaderSize{};
        std::uint32_t Reserved{};
        std::array<char, 64> MapName{};
        std::array<char, 32> StartTime{};
        // What was recorded: "manual", a film or a scenario (D151 sec. 5).
        std::array<char, 64> Source{};
    };

    static_assert(sizeof(TickDumpFileHeader) == 200);

    // Start of one tick record, followed by BridgeTickHeader, BridgeMatch, Players[PlayerCount], Entities[ChangedEntityCount],
    // u32 RemovedHandles[RemovedEntityCount] and Events[NewEventCount], with the sizes of the layout of the file.
    // note: Entities are a delta, because almost every row repeats from one tick to the next (98.9% measured in Forge World): the
    // rows that are new or differ from the previous record, and the handles of the previous record that are gone. A keyframe
    // carries every live row and no removed handle. BridgeTickHeader::EntityCount is the full live count either way.
    struct TickRecordHeader
    {
        std::array<char, 4> Marker{};
        // Match GameTick of the Tick, 0 without a Match.
        std::uint32_t GameTick{};
        // Ticks the AI thread dropped right before this one.
        std::uint32_t DroppedBefore{};
        // Events of this Tick not written by an earlier record.
        std::uint16_t NewEventCount{};
        std::uint16_t Flags{};
        std::uint16_t ChangedEntityCount{};
        std::uint16_t RemovedEntityCount{};
        std::uint32_t Reserved{};
    };

    static_assert(sizeof(TickRecordHeader) == 24);

    // End of a complete .tdump file. A file without it was cut (crash or closed game).
    struct TickDumpFooter
    {
        std::array<char, 4> Marker{};
        std::uint32_t Reserved{};
        std::uint64_t RecordedTicks{};
        std::uint64_t DroppedTicks{};
        // Times the AI thread waited because the queue was full.
        std::uint64_t QueueWaits{};
        // Records with k_TickRecordEventsLost.
        std::uint64_t EventLosses{};
    };

    static_assert(sizeof(TickDumpFooter) == 40);

    // One Tick waiting to be written.
    struct TickDumpEntry
    {
        std::shared_ptr<const Export::Tick::Type::Tick> Snapshot{};
        std::uint32_t DroppedBefore{};
    };
}