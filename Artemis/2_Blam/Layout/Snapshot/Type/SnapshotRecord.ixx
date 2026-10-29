export module Blam.Layout.Snapshot.Type:Record;

import :Raw;
import :Region;
import :Data;

import std;

export namespace Blam::Layout::Snapshot::Type
{
    // Version of the .sdump format (D202). Raised when a header, a block or the order of the blocks changes.
    inline constexpr std::uint32_t k_SnapshotDumpVersion{ 2 };

    // First bytes of a .sdump file.
    inline constexpr std::array<char, 8> k_SnapshotDumpMagic{ 'A', 'S', 'D', 'U', 'M', 'P', '\0', '\0' };

    // Marker of each snapshot record.
    inline constexpr std::array<char, 4> k_SnapshotRecordMarker{ 'S', 'N', 'A', 'P' };

    // Marker of the footer, written when the recording stops.
    inline constexpr std::array<char, 4> k_SnapshotDumpEndMarker{ 'S', 'E', 'N', 'D' };

    // SnapshotRecordHeader::Flags: the first tick that ran after a map load, where a replay can start with empty tables.
    inline constexpr std::uint32_t k_SnapshotFirstAfterLoad{ 1 };

    // SnapshotRecordHeader::Flags: the viewer camera was active in the tick (the collidable service reads it).
    inline constexpr std::uint32_t k_SnapshotViewerActive{ 2 };

    // SnapshotRecordHeader::Flags: Snapshot::HasThreadContext.
    inline constexpr std::uint32_t k_SnapshotHasThreadContext{ 4 };

    // SnapshotRecordHeader::Flags: Snapshot::HasObjectTable.
    inline constexpr std::uint32_t k_SnapshotHasObjectTable{ 8 };

    // Equal bytes between two changes below this length join them into one span.
    inline constexpr std::size_t k_SnapshotSpanGap{ 16 };

    // Start of a .sdump file. The sizes of the regions let a reader refuse a file made with another layout.
    struct SnapshotDumpFileHeader
    {
        std::array<char, 8> Magic{};
        std::uint32_t Version{};
        // Size of this header, so a reader can skip fields it does not know.
        std::uint32_t HeaderSize{};
        // k_BridgeLayoutHash of the .tdump written beside this file.
        std::uint64_t BridgeLayoutHash{};
        std::uint32_t RegionCount{};
        std::array<std::uint32_t, k_RegionCount> RegionSizes{};
        std::array<char, 32> StartTime{};
        // What was recorded, the same text as the .tdump.
        std::array<char, 64> Source{};
    };

    static_assert(sizeof(SnapshotDumpFileHeader) == 160);

    // Start of one record, followed by SnapshotLoadInfo when Flags has k_SnapshotFirstAfterLoad, then ChangedRegionCount region
    // blocks, ChangedObjectCount object blocks in ascending index order and RemovedObjectCount u32 handles.
    // note: Everything is a delta against the Snapshot of the previous record (empty for the first one), compared byte by byte.
    struct SnapshotRecordHeader
    {
        std::array<char, 4> Marker{};
        // Ticks the AI thread dropped right before this one.
        std::uint32_t DroppedBefore{};
        std::uint64_t Generation{};
        std::uint32_t Flags{};
        std::uint16_t ChangedRegionCount{};
        std::uint16_t ChangedObjectCount{};
        std::uint16_t RemovedObjectCount{};
        std::array<std::uint8_t, 6> Reserved{};
    };

    static_assert(sizeof(SnapshotRecordHeader) == 32);

    // Map the recording was loaded from, in the record that has k_SnapshotFirstAfterLoad.
    struct SnapshotLoadInfo
    {
        // File name with extension, without folder.
        std::array<char, 128> MapFileName{};
        std::uint64_t MapFileSize{};
    };

    static_assert(sizeof(SnapshotLoadInfo) == 136);

    // One changed region, followed by SpanCount spans. Size 0 means the region was not captured.
    struct SnapshotRegionBlock
    {
        std::uint64_t OriginalAddress{};
        std::uint32_t Size{};
        std::uint32_t SpanCount{};
        std::uint8_t RegionId{};
        std::array<std::uint8_t, 7> Reserved{};
    };

    static_assert(sizeof(SnapshotRegionBlock) == 24);

    // One new or changed object, followed by SpanCount spans.
    struct SnapshotObjectBlock
    {
        std::uint64_t Address{};
        std::uint32_t Handle{};
        std::uint32_t Size{};
        std::uint32_t SpanCount{};
        std::uint8_t KindId{};
        std::array<std::uint8_t, 3> Reserved{};
    };

    static_assert(sizeof(SnapshotObjectBlock) == 24);

    // Range of bytes that differ from the previous record, followed by Length bytes.
    struct SnapshotSpanHeader
    {
        std::uint32_t Offset{};
        std::uint32_t Length{};
    };

    static_assert(sizeof(SnapshotSpanHeader) == 8);

    // End of a complete .sdump file. A file without it was cut (crash or closed game).
    struct SnapshotDumpFooter
    {
        std::array<char, 4> Marker{};
        // CRC-32 of the map file when HasCrc is 1.
        std::uint32_t MapCrc32{};
        std::uint64_t RecordedTicks{};
        std::uint64_t MapFileSize{};
        // The map loaded when the recording stopped.
        std::array<char, 128> MapFileName{};
        std::uint32_t HasCrc{};
        std::uint32_t Reserved{};
    };

    static_assert(sizeof(SnapshotDumpFooter) == 160);

    // What the writer keeps between records: the Snapshot the next one is compared with.
    struct SnapshotWriterState
    {
        std::shared_ptr<const Snapshot> Previous{};
    };

    // One record read back.
    struct SnapshotRecord
    {
        // The reconstructed Snapshot, identical to the recorded one.
        std::shared_ptr<const Blam::Layout::Snapshot::Type::Snapshot> Snapshot{};
        std::uint64_t Generation{};
        std::uint32_t DroppedBefore{};
        std::uint32_t Flags{};
        // Present in the record that has k_SnapshotFirstAfterLoad.
        std::optional<SnapshotLoadInfo> LoadInfo{};
    };

    // Result of reading the next record.
    enum class SnapshotReadStatus : std::uint8_t
    {
        // A record was read.
        Record,
        // The footer was read: the file is complete.
        End,
        // The file ends without a footer or inside a record.
        Cut,
        // The file is corrupt or made with another layout.
        Error
    };

    struct SnapshotReadResult
    {
        SnapshotReadStatus Status{ SnapshotReadStatus::Error };
        // Set when Status is Record.
        SnapshotRecord Record{};
        // Why the read stopped, when Status is Cut or Error.
        std::string Reason{};
    };

    // What the reader keeps between records.
    struct SnapshotReaderState
    {
        SnapshotDumpFileHeader Header{};
        // The Snapshot of the previous record, the base of the next delta.
        std::shared_ptr<const Snapshot> Previous{};
        // Set once the footer was read.
        std::optional<SnapshotDumpFooter> Footer{};
        // Set once the file header was read and accepted.
        bool IsOpen{};
    };
}