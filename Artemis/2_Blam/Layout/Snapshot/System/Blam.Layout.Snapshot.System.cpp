module Blam.Layout.Snapshot.System;

import Common.Engine.Type;
import Common.Stream.System;
import Common.Text.System;
import std;

namespace
{
    using Common::Stream::System::ReadRaw;
    using Common::Stream::System::ReadValue;
    using Common::Stream::System::WriteRaw;
    using Common::Stream::System::WriteValue;
    using Common::Text::System::CopyText;

    using Blam::Layout::Snapshot::Type::Region;
    using Blam::Layout::Snapshot::Type::RegionId;
    using Blam::Layout::Snapshot::Type::RawObject;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::Layout::Snapshot::Type::SnapshotDumpFileHeader;
    using Blam::Layout::Snapshot::Type::SnapshotDumpFooter;
    using Blam::Layout::Snapshot::Type::SnapshotLoadInfo;
    using Blam::Layout::Snapshot::Type::SnapshotObjectBlock;
    using Blam::Layout::Snapshot::Type::SnapshotReaderState;
    using Blam::Layout::Snapshot::Type::SnapshotReadResult;
    using Blam::Layout::Snapshot::Type::SnapshotReadStatus;
    using Blam::Layout::Snapshot::Type::SnapshotRecordHeader;
    using Blam::Layout::Snapshot::Type::SnapshotRegionBlock;
    using Blam::Layout::Snapshot::Type::SnapshotSpanHeader;
    using Blam::Layout::Snapshot::Type::SnapshotWriterState;

    using Common::Engine::Type::k_HandleIndexMask;
    using Blam::Layout::Snapshot::Type::k_RegionCount;
    using Blam::Layout::Snapshot::Type::k_SnapshotDumpEndMarker;
    using Blam::Layout::Snapshot::Type::k_SnapshotDumpMagic;
    using Blam::Layout::Snapshot::Type::k_SnapshotDumpVersion;
    using Blam::Layout::Snapshot::Type::k_SnapshotFirstAfterLoad;
    using Blam::Layout::Snapshot::Type::k_SnapshotHasObjectTable;
    using Blam::Layout::Snapshot::Type::k_SnapshotHasThreadContext;
    using Blam::Layout::Snapshot::Type::k_SnapshotRecordMarker;
    using Blam::Layout::Snapshot::Type::k_SnapshotSpanGap;
    using Blam::Layout::Snapshot::Type::k_SnapshotViewerActive;

    // Largest region or object a reader accepts, so a corrupt size cannot ask for gigabytes.
    constexpr std::uint32_t k_MaxBlockBytes{ 64 * 1024 * 1024 };

    // Range of a block that differs from the previous record.
    struct Span
    {
        std::size_t Offset{};
        std::size_t Length{};
    };

    // return: Whether all the bytes were read.
    auto GetBytes(std::istream& stream, std::span<std::uint8_t> destination) -> bool
    {
        return ReadRaw(stream, std::as_writable_bytes(destination)) == destination.size();
    }

    // return: The ranges of the bytes that differ, joining the ones with less than k_SnapshotSpanGap equal bytes between them.
    // note: Both spans have the same size.
    auto FindSpans(std::span<const std::uint8_t> previous, std::span<const std::uint8_t> current) -> std::vector<Span>
    {
        std::vector<Span> spans{};
        const std::size_t size{ current.size() };

        std::size_t i{ 0 };
        while (i < size)
        {
            if (previous[i] == current[i])
            {
                ++i;
                continue;
            }

            const std::size_t start{ i };
            std::size_t end{ i + 1 };
            std::size_t scan{ end };
            while (scan < size && scan - end < k_SnapshotSpanGap)
            {
                if (previous[scan] != current[scan]) end = scan + 1;
                ++scan;
            }

            spans.push_back(Span{ .Offset = start, .Length = end - start });
            i = end;
        }

        return spans;
    }

    // return: The spans that rebuild current from previous, one with everything when the size changed or there was no previous.
    auto MakeSpans(std::span<const std::uint8_t> previous, std::span<const std::uint8_t> current) -> std::vector<Span>
    {
        if (current.empty()) return {};
        if (previous.size() != current.size()) return { Span{ .Offset = 0, .Length = current.size() } };
        return FindSpans(previous, current);
    }

    // return: Bytes written.
    auto PutSpans(std::ostream& stream, const std::vector<Span>& spans, std::span<const std::uint8_t> bytes) -> std::uint64_t
    {
        std::uint64_t written{ 0 };
        for (const Span& span : spans)
        {
            written += WriteValue(stream, SnapshotSpanHeader{ .Offset = static_cast<std::uint32_t>(span.Offset),
                .Length = static_cast<std::uint32_t>(span.Length) });
            WriteRaw(stream, std::as_bytes(bytes.subspan(span.Offset, span.Length)));
            written += span.Length;
        }
        return written;
    }

    auto IndexOf(std::uint32_t handle) -> std::uint32_t
    {
        return handle & k_HandleIndexMask;
    }

    // return: Whether a comes before b in ascending index order.
    auto IsBeforeByIndex(std::uint32_t a, std::uint32_t b) -> bool
    {
        return IndexOf(a) != IndexOf(b) ? IndexOf(a) < IndexOf(b) : a < b;
    }

    struct RegionPlan
    {
        std::size_t Index{};
        std::vector<Span> Spans{};
    };

    struct ObjectPlan
    {
        const RawObject* Object{};
        std::vector<Span> Spans{};
    };

    auto Fail(SnapshotReadStatus status, std::string reason) -> SnapshotReadResult
    {
        return SnapshotReadResult{ .Status = status, .Reason = std::move(reason) };
    }

    // Reads the spans of a block into bytes.
    // return: Empty when all fit, else the reason (SnapshotReadStatus::Cut for a short file is told by the stream).
    auto ReadSpans(std::istream& stream, std::uint32_t spanCount, std::vector<std::uint8_t>& bytes, bool& isCut) -> std::string
    {
        for (std::uint32_t i{ 0 }; i < spanCount; ++i)
        {
            SnapshotSpanHeader span{};
            if (!ReadValue(stream, span))
            {
                isCut = true;
                return "the file is cut inside a span header";
            }

            if (static_cast<std::uint64_t>(span.Offset) + span.Length > bytes.size())
            {
                return std::format("a span of {} bytes at {} does not fit in a block of {} bytes", span.Length, span.Offset, bytes.size());
            }

            if (!GetBytes(stream, std::span<std::uint8_t>{ bytes }.subspan(span.Offset, span.Length)))
            {
                isCut = true;
                return "the file is cut inside the bytes of a span";
            }
        }
        return {};
    }
}

namespace Blam::Layout::Snapshot::System
{
    auto WriteFileHeader(std::ostream& stream, std::uint64_t bridgeLayoutHash, std::string_view startTime,
        std::string_view source) -> std::uint64_t
    {
        SnapshotDumpFileHeader header{ .Magic = k_SnapshotDumpMagic, .Version = k_SnapshotDumpVersion,
            .HeaderSize = static_cast<std::uint32_t>(sizeof(SnapshotDumpFileHeader)), .BridgeLayoutHash = bridgeLayoutHash,
            .RegionCount = static_cast<std::uint32_t>(k_RegionCount) };
        for (std::size_t i{ 0 }; i < k_RegionCount; ++i)
        {
            header.RegionSizes[i] = static_cast<std::uint32_t>(Type::RegionSize(static_cast<RegionId>(i)));
        }
        CopyText(startTime, header.StartTime);
        CopyText(source, header.Source);

        return WriteValue(stream, header);
    }

    auto WriteRecord(std::ostream& stream, const std::shared_ptr<const Type::Snapshot>& snapshot, std::uint64_t generation,
        std::uint32_t droppedBefore, std::uint32_t flags, const std::optional<SnapshotLoadInfo>& loadInfo,
        SnapshotWriterState& state) -> std::uint64_t
    {
        static const Region s_NoRegion{};
        const Type::Snapshot* previous{ state.Previous.get() };

        std::vector<RegionPlan> regions{};
        for (std::size_t i{ 0 }; i < k_RegionCount; ++i)
        {
            const Region& current{ snapshot->Regions[i] };
            const Region& before{ previous != nullptr ? previous->Regions[i] : s_NoRegion };
            if (current.OriginalAddress == before.OriginalAddress && current.Bytes == before.Bytes) continue;

            regions.push_back(RegionPlan{ .Index = i, .Spans = MakeSpans(before.Bytes, current.Bytes) });
        }

        std::vector<const RawObject*> sorted{};
        sorted.reserve(snapshot->RawObjects.size());
        for (const auto& [handle, object] : snapshot->RawObjects) sorted.push_back(&object);
        std::ranges::sort(sorted, [](const RawObject* a, const RawObject* b) { return IsBeforeByIndex(a->Handle, b->Handle); });

        std::vector<ObjectPlan> objects{};
        for (const RawObject* object : sorted)
        {
            const RawObject* before{ nullptr };
            if (previous != nullptr)
            {
                if (auto it{ previous->RawObjects.find(object->Handle) }; it != previous->RawObjects.end()) before = &it->second;
            }

            if (before != nullptr && before->Address == object->Address && before->KindId == object->KindId &&
                before->Bytes == object->Bytes) continue;

            const std::span<const std::uint8_t> beforeBytes{ before != nullptr ? std::span<const std::uint8_t>{ before->Bytes } :
                std::span<const std::uint8_t>{} };
            objects.push_back(ObjectPlan{ .Object = object, .Spans = MakeSpans(beforeBytes, object->Bytes) });
        }

        std::vector<std::uint32_t> removed{};
        if (previous != nullptr)
        {
            for (const auto& [handle, object] : previous->RawObjects)
            {
                if (!snapshot->RawObjects.contains(handle)) removed.push_back(handle);
            }
            std::ranges::sort(removed, IsBeforeByIndex);
        }

        constexpr std::size_t k_MaxCount{ (std::numeric_limits<std::uint16_t>::max)() };
        if (objects.size() > k_MaxCount || removed.size() > k_MaxCount)
        {
            stream.setstate(std::ios::failbit);
            return 0;
        }

        std::uint32_t recordFlags{ flags & (k_SnapshotFirstAfterLoad | k_SnapshotViewerActive) };
        if (snapshot->HasThreadContext) recordFlags |= k_SnapshotHasThreadContext;
        if (snapshot->HasObjectTable) recordFlags |= k_SnapshotHasObjectTable;

        std::uint64_t written{ WriteValue(stream, SnapshotRecordHeader{ .Marker = k_SnapshotRecordMarker, .DroppedBefore = droppedBefore,
            .Generation = generation, .Flags = recordFlags, .ChangedRegionCount = static_cast<std::uint16_t>(regions.size()),
            .ChangedObjectCount = static_cast<std::uint16_t>(objects.size()),
            .RemovedObjectCount = static_cast<std::uint16_t>(removed.size()) }) };

        if ((recordFlags & k_SnapshotFirstAfterLoad) != 0) written += WriteValue(stream, loadInfo.value_or(SnapshotLoadInfo{}));

        for (const RegionPlan& plan : regions)
        {
            const Region& region{ snapshot->Regions[plan.Index] };
            written += WriteValue(stream, SnapshotRegionBlock{ .OriginalAddress = region.OriginalAddress,
                .Size = static_cast<std::uint32_t>(region.Bytes.size()), .SpanCount = static_cast<std::uint32_t>(plan.Spans.size()),
                .RegionId = static_cast<std::uint8_t>(plan.Index) });
            written += PutSpans(stream, plan.Spans, region.Bytes);
        }

        for (const ObjectPlan& plan : objects)
        {
            const RawObject& object{ *plan.Object };
            written += WriteValue(stream, SnapshotObjectBlock{ .Address = object.Address, .Handle = object.Handle,
                .Size = static_cast<std::uint32_t>(object.Bytes.size()), .SpanCount = static_cast<std::uint32_t>(plan.Spans.size()),
                .KindId = object.KindId });
            written += PutSpans(stream, plan.Spans, object.Bytes);
        }

        for (const std::uint32_t handle : removed) written += WriteValue(stream, handle);

        if (stream.good()) state.Previous = snapshot;
        return written;
    }

    auto WriteFooter(std::ostream& stream, const SnapshotDumpFooter& footer) -> std::uint64_t
    {
        return WriteValue(stream, footer);
    }

    auto OpenReader(std::istream& stream, SnapshotReaderState& state) -> std::string
    {
        state = SnapshotReaderState{};

        SnapshotDumpFileHeader header{};
        if (!ReadValue(stream, header)) return "the file is shorter than the header of a .sdump";
        if (header.Magic != k_SnapshotDumpMagic) return "the file is not a .sdump (wrong magic)";
        if (header.Version != k_SnapshotDumpVersion)
        {
            return std::format("the version is {}, this reader knows {}", header.Version, k_SnapshotDumpVersion);
        }
        if (header.HeaderSize < sizeof(SnapshotDumpFileHeader)) return std::format("the header size is {}", header.HeaderSize);
        if (header.RegionCount != k_RegionCount)
        {
            return std::format("the file has {} regions, the layout has {}", header.RegionCount, k_RegionCount);
        }
        for (std::size_t i{ 0 }; i < k_RegionCount; ++i)
        {
            const std::size_t expected{ Type::RegionSize(static_cast<RegionId>(i)) };
            if (header.RegionSizes[i] != expected)
            {
                return std::format("region {} has {} bytes in the file, the layout has {}", i, header.RegionSizes[i], expected);
            }
        }

        stream.ignore(static_cast<std::streamsize>(header.HeaderSize - sizeof(SnapshotDumpFileHeader)));
        if (!stream) return "the file is cut inside the header";

        state.Header = header;
        state.IsOpen = true;
        return {};
    }

    auto ReadRecord(std::istream& stream, SnapshotReaderState& state) -> SnapshotReadResult
    {
        if (!state.IsOpen) return Fail(SnapshotReadStatus::Error, "the reader is not open");
        if (state.Footer) return Fail(SnapshotReadStatus::End, {});

        std::array<char, 4> marker{};
        const std::size_t markerBytes{ ReadRaw(stream, std::as_writable_bytes(std::span{ marker })) };
        if (markerBytes == 0) return Fail(SnapshotReadStatus::Cut, "the file ends without a footer");
        if (markerBytes < marker.size()) return Fail(SnapshotReadStatus::Cut, "the file is cut inside a marker");

        if (marker == k_SnapshotDumpEndMarker)
        {
            SnapshotDumpFooter footer{};
            footer.Marker = marker;
            const auto rest{ std::as_writable_bytes(std::span<SnapshotDumpFooter, 1>{ &footer, 1 }).subspan(marker.size()) };
            if (ReadRaw(stream, rest) != rest.size())
            {
                return Fail(SnapshotReadStatus::Cut, "the file is cut inside the footer");
            }
            state.Footer = footer;
            return Fail(SnapshotReadStatus::End, {});
        }

        if (marker != k_SnapshotRecordMarker) return Fail(SnapshotReadStatus::Error, "a record does not start with its marker");

        SnapshotRecordHeader header{};
        header.Marker = marker;
        const auto rest{ std::as_writable_bytes(std::span<SnapshotRecordHeader, 1>{ &header, 1 }).subspan(marker.size()) };
        if (ReadRaw(stream, rest) != rest.size())
        {
            return Fail(SnapshotReadStatus::Cut, "the file is cut inside a record header");
        }

        SnapshotReadResult result{ .Status = SnapshotReadStatus::Record };
        result.Record.Generation = header.Generation;
        result.Record.DroppedBefore = header.DroppedBefore;
        result.Record.Flags = header.Flags;

        if ((header.Flags & k_SnapshotFirstAfterLoad) != 0)
        {
            SnapshotLoadInfo info{};
            if (!ReadValue(stream, info)) return Fail(SnapshotReadStatus::Cut, "the file is cut inside the map information");
            result.Record.LoadInfo = info;
        }

        const Type::Snapshot* previous{ state.Previous.get() };
        auto snapshot{ std::make_shared<Type::Snapshot>() };
        snapshot->HasThreadContext = (header.Flags & k_SnapshotHasThreadContext) != 0;
        snapshot->HasObjectTable = (header.Flags & k_SnapshotHasObjectTable) != 0;
        if (previous != nullptr) snapshot->Regions = previous->Regions;

        bool isCut{ false };

        for (std::uint16_t i{ 0 }; i < header.ChangedRegionCount; ++i)
        {
            SnapshotRegionBlock block{};
            if (!ReadValue(stream, block)) return Fail(SnapshotReadStatus::Cut, "the file is cut inside a region block");
            if (block.RegionId >= k_RegionCount) return Fail(SnapshotReadStatus::Error, std::format("region id {} does not exist", block.RegionId));
            if (block.Size > k_MaxBlockBytes) return Fail(SnapshotReadStatus::Error, std::format("a region of {} bytes is too large", block.Size));

            Region& region{ snapshot->Regions[block.RegionId] };
            region.OriginalAddress = static_cast<std::uintptr_t>(block.OriginalAddress);
            if (region.Bytes.size() != block.Size) region.Bytes.assign(block.Size, 0);

            if (std::string reason{ ReadSpans(stream, block.SpanCount, region.Bytes, isCut) }; !reason.empty())
            {
                return Fail(isCut ? SnapshotReadStatus::Cut : SnapshotReadStatus::Error, std::move(reason));
            }
        }

        std::vector<RawObject> changed{};
        for (std::uint16_t i{ 0 }; i < header.ChangedObjectCount; ++i)
        {
            SnapshotObjectBlock block{};
            if (!ReadValue(stream, block)) return Fail(SnapshotReadStatus::Cut, "the file is cut inside an object block");
            if (block.Size > k_MaxBlockBytes) return Fail(SnapshotReadStatus::Error, std::format("an object of {} bytes is too large", block.Size));

            RawObject object{ .Handle = block.Handle, .Address = static_cast<std::uintptr_t>(block.Address), .KindId = block.KindId };
            if (previous != nullptr)
            {
                if (auto it{ previous->RawObjects.find(block.Handle) }; it != previous->RawObjects.end() && it->second.Bytes.size() == block.Size)
                {
                    object.Bytes = it->second.Bytes;
                }
            }
            if (object.Bytes.size() != block.Size) object.Bytes.assign(block.Size, 0);

            if (std::string reason{ ReadSpans(stream, block.SpanCount, object.Bytes, isCut) }; !reason.empty())
            {
                return Fail(isCut ? SnapshotReadStatus::Cut : SnapshotReadStatus::Error, std::move(reason));
            }
            changed.push_back(std::move(object));
        }

        std::unordered_set<std::uint32_t> gone{};
        for (std::uint16_t i{ 0 }; i < header.RemovedObjectCount; ++i)
        {
            std::uint32_t handle{};
            if (!ReadValue(stream, handle)) return Fail(SnapshotReadStatus::Cut, "the file is cut inside the removed handles");
            gone.insert(handle);
        }
        for (const RawObject& object : changed) gone.insert(object.Handle);

        std::vector<RawObject> objects{};
        if (previous != nullptr)
        {
            for (const auto& [handle, object] : previous->RawObjects)
            {
                if (!gone.contains(handle)) objects.push_back(object);
            }
        }
        for (RawObject& object : changed) objects.push_back(std::move(object));
        std::ranges::sort(objects, [](const RawObject& a, const RawObject& b) { return IsBeforeByIndex(a.Handle, b.Handle); });

        for (RawObject& object : objects)
        {
            const std::uint32_t handle{ object.Handle };
            snapshot->RawObjects.emplace(handle, std::move(object));
        }

        state.Previous = snapshot;
        result.Record.Snapshot = std::move(snapshot);
        return result;
    }
}