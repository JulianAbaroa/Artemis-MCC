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
    using Contract::Bridge::Type::BridgeObjective;
    using Contract::Bridge::Type::BridgeProjectile;
    using Contract::Bridge::Type::BridgeExplosive;
    using Contract::Bridge::Type::BridgeLift;
    using Contract::Bridge::Type::BridgeStand;
    using Contract::Bridge::Type::BridgeWeapon;
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
            (header.NewEventCount * sizeof(BridgeEvent)) + (tick.ObjectiveCount * sizeof(BridgeObjective)) +
            (tick.StandCount * sizeof(BridgeStand)) + (tick.WeaponCount * sizeof(BridgeWeapon)) +
            (tick.ProjectileCount * sizeof(BridgeProjectile)) + (tick.ExplosiveCount * sizeof(BridgeExplosive)) +
            (tick.LiftCount * sizeof(BridgeLift)) };

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
        // The recording cannot be replayed in this build (it does not start at a map load).
        Skipped,
        // The .tdump has another layout than this build (Bridge.xml changed): the .sdump was replayed, Replayed has the records in the
        // layout of this build and nothing was compared. The golden has to be approved again.
        LayoutChanged,
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
        // The records the pipeline gave, as they were written (without the header of the file). Status Equal, Differs or LayoutChanged.
        std::vector<char> Replayed{};
    };

    // Compares the records the pipeline gives with the .tdump recorded in the same session, one record at a time (D204).
    // Use: Begin, Start with the flags of the first record of the .sdump, OnRecord after the pipeline ran each record, End.
    // note: The comparer never runs the pipeline. Once IsFinal is true, the result does not change and the other calls do nothing.
    class ReplayComparer
    {
    public:
        explicit ReplayComparer(std::istream& tdump) : m_Tdump{ tdump }
        {
        }

        // Reads the header of the .tdump.
        // return: false when the result is final already (the header is not valid).
        auto Begin() -> bool
        {
            TickDumpFileHeader golden{};
            m_Tdump.read(reinterpret_cast<char*>(&golden), sizeof(golden));
            if (m_Tdump.gcount() != static_cast<std::streamsize>(sizeof(golden)) || golden.Magic != k_TickDumpMagic ||
                golden.HeaderSize < sizeof(golden))
            {
                this->Fail("the .tdump does not start with a valid header");
                return false;
            }
            m_Tdump.ignore(static_cast<std::streamsize>(golden.HeaderSize - sizeof(golden)) + golden.LayoutJsonSize);
            if (!m_Tdump)
            {
                this->Fail("the .tdump is cut inside its header");
                return false;
            }

            // note: The .sdump holds engine snapshots, which do not depend on the layout of the bridge, so another layout of the .tdump is
            // not a reason to skip: the records are replayed and the golden is reported as changed. The layout hash the .sdump carries
            // (the one of the .tdump written beside it) is not compared: a golden approved again after a layout change has a new hash and
            // keeps the .sdump of the session.
            m_GoldenLayoutHash = golden.LayoutHash;
            m_IsOtherLayout = golden.LayoutHash != k_BridgeLayoutHash;

            Bridge::Codec::Dump::System::Begin(m_Writer);
            m_Flat = std::make_unique<BridgeTick>();
            return true;
        }

        // Checks that the .sdump starts at a map load.
        // param firstFlags: SnapshotRecordHeader::Flags of the first record.
        // return: false when the result is final already (the recording is skipped).
        auto Start(std::uint32_t firstFlags) -> bool
        {
            if ((firstFlags & k_SnapshotFirstAfterLoad) != 0) return true;

            m_Result.Status = ReplayStatus::Skipped;
            m_Result.Reason = "the recording does not start at the first tick after a map load";
            m_IsFinal = true;
            return false;
        }

        // Flattens the Tick the pipeline gave for the record and compares it with the next record of the .tdump.
        // param exportLayer: The Export layer the pipeline of the record ran in.
        auto OnRecord(Bridge::Codec::Layer& exportLayer, std::uint32_t droppedBefore) -> void
        {
            if (m_IsFinal) return;

            const auto tick{ exportLayer.m_TickStore.Acquire() };

            // The same header the recorder flattened with: the clock fields belong to the bridge.
            Bridge::Codec::Dump::System::ClearHeader(*m_Flat);
            exportLayer.m_FlatService.Flatten(*tick, *m_Flat);

            std::ostringstream buffer{};
            Bridge::Codec::Dump::System::WriteRecord(buffer, *m_Flat, tick->Match != nullptr, droppedBefore, m_Writer);
            const std::string replayed{ buffer.str() };
            m_Result.Replayed.insert(m_Result.Replayed.end(), replayed.begin(), replayed.end());

            if (!m_IsOtherLayout && ReadGoldenRecord(m_Tdump, m_Recorded) != GoldenStatus::Record)
            {
                this->Fail(std::format("the .tdump has {} records and the .sdump has more", m_Result.Records));
                return;
            }

            ++m_Result.Records;
            if (!m_IsOtherLayout && m_Result.FirstDifference == 0 && !std::ranges::equal(m_Recorded, replayed))
            {
                m_Result.FirstDifference = m_Result.Records;
                std::memcpy(&m_Result.GameTick, m_Recorded.data() + k_GameTickOffset, sizeof(m_Result.GameTick));
            }
        }

        // Ends the result with an error.
        auto Fail(std::string reason) -> void
        {
            m_Result.Status = ReplayStatus::Error;
            m_Result.Reason = std::move(reason);
            m_IsFinal = true;
        }

        auto IsFinal() const -> bool
        {
            return m_IsFinal;
        }

        // Closes the comparison.
        // param status: How the read of the .sdump stopped (Record when the replay stopped at the next map load).
        // param reason: SnapshotReadResult::Reason of that read.
        // return: The result. Replayed holds the records the pipeline gave unless the result is final.
        auto End(SnapshotReadStatus status, const std::string& reason) -> ReplayResult
        {
            if (!m_IsFinal)
            {
                if (status == SnapshotReadStatus::Error)
                {
                    this->Fail("the .sdump is corrupt after " + std::to_string(m_Result.Records) + " records: " + reason);
                }
                else
                {
                    if (status == SnapshotReadStatus::Cut) m_Result.Reason = "the .sdump is cut: " + reason;

                    if (m_IsOtherLayout)
                    {
                        m_Result.Status = ReplayStatus::LayoutChanged;
                        m_Result.Reason = std::format("layout hash {:016X} in the .tdump, {:016X} in this build", m_GoldenLayoutHash, k_BridgeLayoutHash);
                    }
                    else
                    {
                        m_Result.Status = m_Result.FirstDifference == 0 ? ReplayStatus::Equal : ReplayStatus::Differs;
                    }
                    m_IsFinal = true;
                }
            }

            return std::move(m_Result);
        }

    private:
        std::istream& m_Tdump;
        ReplayResult m_Result{};
        TickDumpWriterState m_Writer{};
        std::unique_ptr<BridgeTick> m_Flat{};
        std::vector<char> m_Recorded{};
        std::uint64_t m_GoldenLayoutHash{};
        bool m_IsOtherLayout{ false };
        bool m_IsFinal{ false };
    };

    // Replays a .sdump through the pipeline and compares each record with the .tdump recorded in the same session (D204).
    // param exportLayer: The Export layer of layers that are built and have their map loaded. The other layers are the ones it
    // was built with.
    // note: Stops at the end of the file or at the next record that has k_SnapshotFirstAfterLoad, where the game loaded another map.
    auto Replay(Bridge::Codec::Layer& exportLayer, std::istream& sdump, std::istream& tdump) -> ReplayResult
    {
        ReplayComparer comparer{ tdump };
        if (!comparer.Begin()) return comparer.End(SnapshotReadStatus::Record, {});

        SnapshotReaderState reader{};
        if (const std::string reason{ ::Blam::Layout::Snapshot::System::OpenReader(sdump, reader) }; !reason.empty())
        {
            comparer.Fail("the .sdump cannot be read: " + reason);
            return comparer.End(SnapshotReadStatus::Record, {});
        }

        auto next{ ::Blam::Layout::Snapshot::System::ReadRecord(sdump, reader) };
        if (next.Status != SnapshotReadStatus::Record)
        {
            comparer.Fail(next.Reason.empty() ? "the .sdump has no records" : "the .sdump has no records: " + next.Reason);
            return comparer.End(SnapshotReadStatus::Record, {});
        }
        if (!comparer.Start(next.Record.Flags)) return comparer.End(SnapshotReadStatus::Record, {});

        for (bool isFirst{ true }; next.Status == SnapshotReadStatus::Record && !comparer.IsFinal(); isFirst = false)
        {
            const auto& record{ next.Record };
            if (!isFirst && (record.Flags & k_SnapshotFirstAfterLoad) != 0) break;

            exportLayer.m_PipelineService.Run(*record.Snapshot, record.Generation, {});
            comparer.OnRecord(exportLayer, record.DroppedBefore);

            next = ::Blam::Layout::Snapshot::System::ReadRecord(sdump, reader);
        }

        return comparer.End(next.Status, next.Reason);
    }
}