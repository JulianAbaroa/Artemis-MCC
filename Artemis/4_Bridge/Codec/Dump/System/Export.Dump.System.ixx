export module Export.Dump.System;

import Contract.Bridge;
import Export.Dump.Type;
import std;

export namespace Export::Dump::System
{
    // Writes the header of a .tdump file (D151): the file header and the JSON of the layout of the BridgeTick.
    // return: Bytes written.
    auto WriteFileHeader(std::ostream& stream, std::string_view mapName, std::string_view startTime, std::string_view source) -> std::uint64_t;

    // Zeroes the header of the BridgeTick, padding included, before it is flattened for a record.
    // note: The clock fields of the header belong to the bridge (BridgeService::EndTick), so a dump has them in zero. A header
    // cleared by assignment can keep stack bytes in its padding, and records must be equal byte by byte.
    auto ClearHeader(Platform::Bridge::Type::BridgeTick& flat) -> void;

    // Starts a recording: the first record is a keyframe.
    auto Begin(Type::TickDumpWriterState& state) -> void;

    // Writes one record: the BridgeTick with the entities and the events as a delta against the previous record.
    // param flat: The BridgeTick flattened with an empty header (the clock fields belong to the bridge).
    // param hasMatch: The Tick had a Match, so its GameTick is valid.
    // param droppedBefore: Ticks the AI thread dropped right before this one.
    // return: The bytes written and whether events were lost.
    // note: The state moves to the next record only if the stream is still good afterwards.
    auto WriteRecord(std::ostream& stream, const Platform::Bridge::Type::BridgeTick& flat, bool hasMatch, std::uint32_t droppedBefore,
        Type::TickDumpWriterState& state) -> Type::TickDumpWriteResult;

    // Writes the footer of a complete .tdump file.
    // return: Bytes written.
    auto WriteFooter(std::ostream& stream, const Type::TickDumpCounters& counters) -> std::uint64_t;
}