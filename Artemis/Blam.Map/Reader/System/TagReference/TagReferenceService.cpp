module Map.Reader.System;
import :TagReference;

import Common.Engine.Type;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

namespace Map::Reader::System
{
    auto TagReferenceService::GetTagOffset(std::int32_t tagIndex) const -> std::int64_t
    {
        if (tagIndex < 0 || tagIndex >=
            (std::int32_t)m_TagIndexStore.GetTagsSize())
        {
            return -1;
        }

        return this->ResolveTagOffset(m_TagIndexStore.GetTag(tagIndex));
    }

    auto TagReferenceService::ResolveTagOffset(
        const TagTableEntry& tag) const -> std::int64_t
    {
        if (tag.MemoryAddress == 0) return -1;

        return m_FormulaService.ToFileOffset(m_FormulaService.Expand(tag.MemoryAddress));
    }

    auto TagReferenceService::ResolveTagReferenceName(
        const TagReference& ref) const -> std::string
    {
        if (ref.DatumIndex == k_NoHandle || ref.DatumIndex == 0)
        {
            return {};
        }

        std::int32_t tagIndex = ref.DatumIndex & 0xFFFF;
        if (tagIndex < 0 || tagIndex >=
            (std::int32_t)m_TagIndexStore.GetTagsSize())
        {
            return {};
        }

        return m_TagIndexStore.GetTagName(tagIndex);
    }

    auto TagReferenceService::ResolveBlockOffset(
        const TagBlock& block) const -> std::int64_t
    {
        if (block.EntryCount <= 0 || block.Pointer == 0) return -1;

        return m_FormulaService.ToFileOffset(m_FormulaService.Expand(block.Pointer));
    }
}