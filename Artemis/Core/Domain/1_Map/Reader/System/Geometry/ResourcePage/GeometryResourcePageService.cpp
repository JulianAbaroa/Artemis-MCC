module Map.Reader.System;
import :Geometry.ResourcePage;

namespace
{
    using Map::Tag::Type::Play::Structure::Play_SegmentsEntry;
    using Map::Tag::Type::Play::Structure::Play_RawPagesEntry;
}

namespace Map::Reader::System
{
    auto GeometryResourcePageService::Read(std::uint32_t datum,
        const Zone_TagResourcesObject** outEntry, const ZoneObject* zone,
        const PlayObject* play) const -> std::vector<std::uint8_t>
    {
        int resourceIdx = (int)(datum & 0xFFFF);
        if (resourceIdx < 0 || resourceIdx >= (int)zone->TagResources.size())
        {
            return {};
        }

        const Zone_TagResourcesObject& entry = zone->TagResources[resourceIdx];
        if (outEntry) *outEntry = &entry;

        int segmentIdx = (int)entry.PlaySegmentIndex;
        if (segmentIdx < 0 || segmentIdx >= (int)play->Segments.size())
        {
            return {};
        }

        const Play_SegmentsEntry& segment = play->Segments[segmentIdx];

        int pageIdx = (int)segment.PrimaryPageIndex;
        std::int32_t segmentOffset = segment.PrimarySegmentOffset;

        if (segment.SecondaryPageIndex >= 0)
        {
            pageIdx = (int)segment.SecondaryPageIndex;
            segmentOffset = segment.SecondarySegmentOffset;
        }

        if (pageIdx < 0 || pageIdx >= (int)play->RawPages.size())
        {
            return {};
        }

        const Play_RawPagesEntry* page = &play->RawPages[pageIdx];

        if (page->BlockOffset == 0xFFFFFFFFu ||
            page->CompressedBlockSize == 0)
        {
            pageIdx = (int)segment.PrimaryPageIndex;
            segmentOffset = segment.PrimarySegmentOffset;

            if (pageIdx < 0 || pageIdx >= (int)play->RawPages.size())
            {
                return {};
            }

            page = &play->RawPages[pageIdx];
        }

        if (page->SharedCacheIndex >= 0)
        {
            std::string externalPath{};

            int externalIdx = (int)page->SharedCacheIndex;
            if (externalIdx < (int)play->ExternalCacheReferences.size())
            {
                const char* mapPath = reinterpret_cast<const char*>(
                    &play->ExternalCacheReferences[externalIdx].MapPath);

                externalPath = m_FileLocatorService.ResolveExternalCachePath(mapPath);
            }

            if (externalPath.empty())
            {
                m_Log.Message("[GeometryResourcePageService] WARNING:"
                    " External cache resource without a resolvable path (idx={}).", externalIdx);
                return {};
            }

            std::int64_t externalFilePosition = m_FormulaService.
                ToResourceOffset((std::int64_t)page->BlockOffset);

            std::int32_t externalSegmentLength =
                (std::int32_t)page->UncompressedBlockSize - segmentOffset;
            if (externalSegmentLength <= 0) return {};

            return m_DataStreamService.ReadSegment(
                externalPath, externalFilePosition,
                (std::int32_t)page->CompressedBlockSize,
                (std::int32_t)page->UncompressedBlockSize,
                segmentOffset, externalSegmentLength);
        }

        std::int64_t filePosition = m_FormulaService.
            ToResourceOffset((std::int64_t)page->BlockOffset);

        std::int32_t segmentLength =
            (std::int32_t)page->UncompressedBlockSize - segmentOffset;
        if (segmentLength <= 0) return {};

        return m_DataStreamService.ReadSegment(
            m_FileStore.GetMapFilePath(), filePosition,
            (std::int32_t)page->CompressedBlockSize,
            (std::int32_t)page->UncompressedBlockSize,
            segmentOffset, segmentLength);
    }
}