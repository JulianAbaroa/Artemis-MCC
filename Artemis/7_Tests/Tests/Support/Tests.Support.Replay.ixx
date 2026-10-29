export module Tests.Support.Replay;

import Contract.Bridge.Type;
import Bridge.Codec.Dump.Type;
import Bridge.Codec.Dump.System;
import Bridge.Codec.Layer;
import Bridge.Codec.Tick.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Layout.Snapshot.System;
import std;

namespace
{
    using Contract::Bridge::Type::BridgeEntity;
    using Contract::Bridge::Type::BridgeEvent;
    using Contract::Bridge::Type::BridgeMatch;
    using Contract::Bridge::Type::BridgePlayer;
    using Contract::Bridge::Type::BridgeTick;
    using Contract::Bridge::Type::BridgeTickHeader;

    using Bridge::Codec::Dump::Type::TickDumpFileHeader;
    using Bridge::Codec::Dump::Type::TickDumpWriterState;
    using Bridge::Codec::Dump::Type::TickRecordHeader;
    using ::Blam::Layout::Snapshot::Type::SnapshotReaderState;
    using ::Blam::Layout::Snapshot::Type::SnapshotReadStatus;

    using Contract::Bridge::Type::k_BridgeLayoutHash;
    using Bridge::Codec::Dump::Type::k_TickDumpEndMarker;
    using Bridge::Codec::Dump::Type::k_TickDumpMagic;
    using Bridge::Codec::Dump::Type::k_TickRecordMarker;
    using ::Blam::Layout::Snapshot::Type::k_SnapshotFirstAfterLoad;

    // Where a read of the recorded .tdump stopped.
    enum class GoldenStatus : std::uint8_t
    {
        Record,
        End,
        Error
    };

    // Offset of GameTick inside a record: after the marker.
    constexpr std::size_t k_GameTickOffset{ sizeof(TickRecordHeader::Marker) };

    // Reads the next record of the recorded .tdump, byte by byte, with the sizes of the layout of this build.
    auto ReadGoldenRecord(std::istream& tdump, std::vector<char>& bytes) -> GoldenStatus
    {
        TickRecordHeader header{};
        tdump.read(reinterpret_cast<char*>(&header), sizeof(header));
        const std::streamsize got{ tdump.gcount() };
        if (got == 0) return GoldenStatus::End;
        if (got >= static_cast<std::streamsize>(header.Marker.size()) && header.Marker == k_TickDumpEndMarker) return GoldenStatus::End;
        if (got != static_cast<std::streamsize>(sizeof(header)) || header.Marker != k_TickRecordMarker) return GoldenStatus::Error;

        BridgeTickHeader tick{};
        tdump.read(reinterpret_cast<char*>(&tick), sizeof(tick));
        if (tdump.gcount() != static_cast<std::streamsize>(sizeof(tick))) return GoldenStatus::Error;

        const std::size_t rest{ sizeof(BridgeMatch) + (tick.PlayerCount * sizeof(BridgePlayer)) +
            (header.ChangedEntityCount * sizeof(BridgeEntity)) + (header.RemovedEntityCount * sizeof(std::uint32_t)) +
            (header.NewEventCount * sizeof(BridgeEvent)) };

        bytes.resize(sizeof(header) + sizeof(tick) + rest);
        std::memcpy(bytes.data(), &header, sizeof(header));
        std::memcpy(bytes.data() + sizeof(header), &tick, sizeof(tick));
        tdump.read(bytes.data() + sizeof(header) + sizeof(tick), static_cast<std::streamsize>(rest));
        if (tdump.gcount() != static_cast<std::streamsize>(rest)) return GoldenStatus::Error;

        return GoldenStatus::Record;
    }
}

export namespace Tests::Support::Replay
{
    enum class ReplayStatus : std::uint8_t
    {
        // Every record the .sdump replays gives the same bytes as the recorded .tdump.
        Equal,
        // A record differs: FirstDifference.
        Differs,
        // The recording cannot be replayed in this build (another layout, or it does not start at a map load).
        Skipped,
        // A file is corrupt or the two do not match.
        Error
    };

    struct ReplayResult
    {
        ReplayStatus Status{ ReplayStatus::Error };
        // Records compared.
        std::size_t Records{};
        // Number of the first record that differs, counted from 1. 0 when none.
        std::size_t FirstDifference{};
        // GameTick of that record in the recorded .tdump.
        std::uint32_t GameTick{};
        // Why the status is Skipped or Error, or a note when the .sdump is cut.
        std::string Reason{};
        // The records the pipeline gave, as they were written (without the header of the file).
        std::vector<char> Replayed{};
    };

    // Replays a .sdump through the pipeline and compares each record with the .tdump recorded in the same session (D204).
    // param exportLayer: The Export layer of layers that are built and have their map loaded. The other layers are the ones it
    // was built with.
    // note: Stops at the end of the file or at the next record that has k_SnapshotFirstAfterLoad, where the game loaded another map.
    auto Replay(Bridge::Codec::Layer& exportLayer, std::istream& sdump, std::istream& tdump) -> ReplayResult
    {
        ReplayResult result{};

        TickDumpFileHeader golden{};
        tdump.read(reinterpret_cast<char*>(&golden), sizeof(golden));
        if (tdump.gcount() != static_cast<std::streamsize>(sizeof(golden)) || golden.Magic != k_TickDumpMagic ||
            golden.HeaderSize < sizeof(golden))
        {
            result.Reason = "the .tdump does not start with a valid header";
            return result;
        }
        tdump.ignore(static_cast<std::streamsize>(golden.HeaderSize - sizeof(golden)) + golden.LayoutJsonSize);
        if (!tdump)
        {
            result.Reason = "the .tdump is cut inside its header";
            return result;
        }

        SnapshotReaderState reader{};
        if (const std::string reason{ ::Blam::Layout::Snapshot::System::OpenReader(sdump, reader) }; !reason.empty())
        {
            result.Reason = "the .sdump cannot be read: " + reason;
            return result;
        }

        if (golden.LayoutHash != k_BridgeLayoutHash || reader.Header.BridgeLayoutHash != golden.LayoutHash)
        {
            result.Status = ReplayStatus::Skipped;
            result.Reason = std::format("layout hash {:016X} in the .tdump, {:016X} in the .sdump, {:016X} in this build",
                golden.LayoutHash, reader.Header.BridgeLayoutHash, k_BridgeLayoutHash);
            return result;
        }

        auto next{ ::Blam::Layout::Snapshot::System::ReadRecord(sdump, reader) };
        if (next.Status != SnapshotReadStatus::Record)
        {
            result.Reason = next.Reason.empty() ? "the .sdump has no records" : "the .sdump has no records: " + next.Reason;
            return result;
        }
        if ((next.Record.Flags & k_SnapshotFirstAfterLoad) == 0)
        {
            result.Status = ReplayStatus::Skipped;
            result.Reason = "the recording does not start at the first tick after a map load";
            return result;
        }

        TickDumpWriterState writer{};
        Bridge::Codec::Dump::System::Begin(writer);
        auto flat{ std::make_unique<BridgeTick>() };
        std::vector<char> recorded{};

        for (bool isFirst{ true }; next.Status == SnapshotReadStatus::Record; isFirst = false)
        {
            const auto& record{ next.Record };
            if (!isFirst && (record.Flags & k_SnapshotFirstAfterLoad) != 0) break;

            exportLayer.m_PipelineService.Run(*record.Snapshot, record.Generation, {});
            const auto tick{ exportLayer.m_TickStore.Acquire() };

            // The same header the recorder flattened with: the clock fields belong to the bridge.
            Bridge::Codec::Dump::System::ClearHeader(*flat);
            exportLayer.m_FlatService.Flatten(*tick, *flat);

            std::ostringstream buffer{};
            Bridge::Codec::Dump::System::WriteRecord(buffer, *flat, tick->Match != nullptr, record.DroppedBefore, writer);
            const std::string replayed{ buffer.str() };
            result.Replayed.insert(result.Replayed.end(), replayed.begin(), replayed.end());

            if (ReadGoldenRecord(tdump, recorded) != GoldenStatus::Record)
            {
                result.Reason = std::format("the .tdump has {} records and the .sdump has more", result.Records);
                return result;
            }

            ++result.Records;
            if (result.FirstDifference == 0 && !std::ranges::equal(recorded, replayed))
            {
                result.FirstDifference = result.Records;
                std::memcpy(&result.GameTick, recorded.data() + k_GameTickOffset, sizeof(result.GameTick));
            }

            next = ::Blam::Layout::Snapshot::System::ReadRecord(sdump, reader);
        }

        if (next.Status == SnapshotReadStatus::Error)
        {
            result.Reason = "the .sdump is corrupt after " + std::to_string(result.Records) + " records: " + next.Reason;
            return result;
        }
        if (next.Status == SnapshotReadStatus::Cut) result.Reason = "the .sdump is cut: " + next.Reason;

        result.Status = result.FirstDifference == 0 ? ReplayStatus::Equal : ReplayStatus::Differs;
        return result;
    }
}