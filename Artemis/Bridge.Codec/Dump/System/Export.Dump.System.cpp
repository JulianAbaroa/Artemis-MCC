module Export.Dump.System;

import Common.Stream.System;
import Common.Text.System;
import std;

namespace
{
    using Common::Stream::System::WriteRows;
    using Common::Stream::System::WriteValue;
    using Common::Text::System::CopyText;

    using Platform::Bridge::Type::BridgeTick;
    using Platform::Bridge::Type::BridgeEvent;
    using Platform::Bridge::Type::BridgePlayer;
    using Platform::Bridge::Type::BridgeEntity;

    using Export::Dump::Type::TickDumpFileHeader;
    using Export::Dump::Type::TickDumpFooter;
    using Export::Dump::Type::TickDumpWriterState;
    using Export::Dump::Type::TickRecordHeader;

    using Platform::Bridge::Type::k_BridgeLayoutJson;
    using Platform::Bridge::Type::k_BridgeLayoutHash;
    using Platform::Bridge::Type::k_BridgeLayoutVersion;
    using Export::Dump::Type::k_KeyframeInterval;
    using Export::Dump::Type::k_TickDumpEndMarker;
    using Export::Dump::Type::k_TickDumpMagic;
    using Export::Dump::Type::k_TickDumpVersion;
    using Export::Dump::Type::k_TickRecordEventsLost;
    using Export::Dump::Type::k_TickRecordHasMatch;
    using Export::Dump::Type::k_TickRecordKeyframe;
    using Export::Dump::Type::k_TickRecordMarker;
}

namespace Export::Dump::System
{
    auto WriteFileHeader(std::ostream& stream, std::string_view mapName, std::string_view startTime, std::string_view source) -> std::uint64_t
    {
        TickDumpFileHeader header{ .Magic = k_TickDumpMagic, .Version = k_TickDumpVersion,
            .HeaderSize = static_cast<std::uint32_t>(sizeof(TickDumpFileHeader)), .LayoutHash = k_BridgeLayoutHash,
            .LayoutVersion = k_BridgeLayoutVersion, .LayoutJsonSize = static_cast<std::uint32_t>(k_BridgeLayoutJson.size()),
            .RecordHeaderSize = static_cast<std::uint32_t>(sizeof(TickRecordHeader)) };
        CopyText(mapName, header.MapName);
        CopyText(startTime, header.StartTime);
        CopyText(source, header.Source);

        std::uint64_t bytes{ WriteValue(stream, header) };
        stream.write(k_BridgeLayoutJson.data(), static_cast<std::streamsize>(k_BridgeLayoutJson.size()));
        bytes += k_BridgeLayoutJson.size();
        return bytes;
    }

    auto ClearHeader(BridgeTick& flat) -> void
    {
        std::memset(&flat.Header, 0, sizeof(flat.Header));
    }

    auto Begin(TickDumpWriterState& state) -> void
    {
        state = TickDumpWriterState{ .RecordsSinceKeyframe = k_KeyframeInterval };
    }

    auto WriteRecord(std::ostream& stream, const BridgeTick& flat, bool hasMatch, std::uint32_t droppedBefore,
        TickDumpWriterState& state) -> Type::TickDumpWriteResult
    {
        Type::TickDumpWriteResult result{};

        TickRecordHeader record{ .Marker = k_TickRecordMarker, .DroppedBefore = droppedBefore };
        if (hasMatch)
        {
            record.GameTick = flat.Match.GameTick;
            record.Flags |= k_TickRecordHasMatch;
        }

        // The BridgeTick carries the newest events of the match (oldest first, Sequence from 1 in each match), so each record
        // writes only the ones not written yet. A smaller LastEventSequence means a new match.
        std::uint32_t last{ state.LastEventSequence };
        if (flat.Header.LastEventSequence < last) last = 0;
        const std::span<const BridgeEvent> window{ flat.Events.data(), flat.Header.EventCount };
        std::size_t first{ 0 };
        while (first < window.size() && window[first].Sequence <= last) ++first;
        const std::span<const BridgeEvent> fresh{ window.subspan(first) };
        if (last != 0 && !fresh.empty() && fresh.front().Sequence != last + 1)
        {
            record.Flags |= k_TickRecordEventsLost;
            result.IsEventsLost = true;
        }
        record.NewEventCount = static_cast<std::uint16_t>(fresh.size());

        // Entities against the previous record. Both are sorted by handle (Environment Entities), so one merge finds the rows that
        // are new or changed and the handles that are gone.
        const std::span<const BridgeEntity> current{ flat.Entities.data(), flat.Header.EntityCount };
        const std::vector<BridgeEntity>& previous{ state.PreviousEntities };
        const bool isKeyframe{ state.RecordsSinceKeyframe >= k_KeyframeInterval };
        std::vector<BridgeEntity> changed{};
        std::vector<std::uint32_t> removed{};
        if (isKeyframe)
        {
            record.Flags |= k_TickRecordKeyframe;
            changed.assign(current.begin(), current.end());
        }
        else
        {
            std::size_t i{ 0 };
            std::size_t j{ 0 };
            while (i < previous.size() || j < current.size())
            {
                if (j == current.size() || (i < previous.size() && previous[i].Handle < current[j].Handle))
                {
                    removed.push_back(previous[i++].Handle);
                }
                else if (i == previous.size() || current[j].Handle < previous[i].Handle)
                {
                    changed.push_back(current[j++]);
                }
                else
                {
                    if (std::memcmp(&previous[i], &current[j], sizeof(BridgeEntity)) != 0) changed.push_back(current[j]);
                    ++i;
                    ++j;
                }
            }
        }
        record.ChangedEntityCount = static_cast<std::uint16_t>(changed.size());
        record.RemovedEntityCount = static_cast<std::uint16_t>(removed.size());

        std::uint64_t bytes{ WriteValue(stream, record) };
        bytes += WriteValue(stream, flat.Header);
        bytes += WriteValue(stream, flat.Match);
        bytes += WriteRows(stream, std::span<const BridgePlayer>{ flat.Players.data(), flat.Header.PlayerCount });
        bytes += WriteRows(stream, std::span<const BridgeEntity>{ changed });
        bytes += WriteRows(stream, std::span<const std::uint32_t>{ removed });
        bytes += WriteRows(stream, fresh);
        result.Bytes = bytes;

        if (stream.good())
        {
            state.PreviousEntities.assign(current.begin(), current.end());
            state.RecordsSinceKeyframe = isKeyframe ? 1 : state.RecordsSinceKeyframe + 1;
            state.LastEventSequence = flat.Header.LastEventSequence;
        }

        return result;
    }

    auto WriteFooter(std::ostream& stream, const Type::TickDumpCounters& counters) -> std::uint64_t
    {
        return WriteValue(stream, TickDumpFooter{ .Marker = k_TickDumpEndMarker, .RecordedTicks = counters.RecordedTicks,
            .DroppedTicks = counters.DroppedTicks, .QueueWaits = counters.QueueWaits, .EventLosses = counters.EventLosses });
    }
}