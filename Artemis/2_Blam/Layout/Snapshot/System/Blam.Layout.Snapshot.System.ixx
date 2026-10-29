export module Blam.Layout.Snapshot.System;

import Blam.Layout.Snapshot.Type;
import std;

export namespace Blam::Layout::Snapshot::System
{
    // Writes the header of a .sdump file (D202).
    // param bridgeLayoutHash: The hash of the layout of the .tdump written beside this file.
    // return: Bytes written.
    auto WriteFileHeader(std::ostream& stream, std::uint64_t bridgeLayoutHash, std::string_view startTime,
        std::string_view source) -> std::uint64_t;

    // Writes one record: what changed in the snapshot against the one of the previous record, byte by byte.
    // param flags: k_SnapshotFirstAfterLoad and k_SnapshotViewerActive. The snapshot sets the other flags.
    // param loadInfo: Written only with k_SnapshotFirstAfterLoad.
    // return: Bytes written.
    // note: The state keeps the snapshot as the base of the next record only if the stream is still good afterwards.
    auto WriteRecord(std::ostream& stream, const std::shared_ptr<const Type::Snapshot>& snapshot, std::uint64_t generation,
        std::uint32_t droppedBefore, std::uint32_t flags, const std::optional<Type::SnapshotLoadInfo>& loadInfo,
        Type::SnapshotWriterState& state) -> std::uint64_t;

    // Writes the footer of a complete .sdump file.
    // return: Bytes written.
    auto WriteFooter(std::ostream& stream, const Type::SnapshotDumpFooter& footer) -> std::uint64_t;

    // Reads and checks the header of a .sdump file.
    // return: Empty when the file can be read, else why not (not a .sdump, other version, other layout of the regions).
    auto OpenReader(std::istream& stream, Type::SnapshotReaderState& state) -> std::string;

    // Reads the next record and rebuilds its snapshot, identical to the recorded one, with the objects inserted in ascending
    // index order as the capture does.
    // return: The record, or why there is none: footer read (End), file cut (Cut) or corrupt (Error). Never throws on bad data.
    // note: After End the footer is in the state.
    auto ReadRecord(std::istream& stream, Type::SnapshotReaderState& state) -> Type::SnapshotReadResult;
}