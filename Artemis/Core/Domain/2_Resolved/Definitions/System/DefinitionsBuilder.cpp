module Resolved.Definitions.System;

import Common.Math.Type;
import Map.Reader.State;
import Map.Reader.Type;
import Map.Tag.Type;

namespace
{
    using Common::Math::Type::Triangle;
    using Map::Reader::State::TagStore;
    using Map::Reader::System::GroupDescriptor;
    using Map::Reader::Type::Geometry::SbspGeometry;
    using Map::Tag::Type::Sbsp::Object::SbspObject;

    // Reads the map object and the definition out of the signature of a builder Build method.
    template <typename TMethod>
    struct BuildTraits;

    template <typename TBuilder, typename TDefinition, typename TMapObject>
    struct BuildTraits<TDefinition (TBuilder::*)(const TMapObject&)>
    {
        using Definition = TDefinition;
        using MapObject = TMapObject;
    };

    // Spells the four characters of a tag group magic.
    auto FourCC(std::uint32_t magic) -> std::string
    {
        return std::string{
            static_cast<char>(magic >> 24),
            static_cast<char>(magic >> 16),
            static_cast<char>(magic >> 8),
            static_cast<char>(magic) };
    }

    // Adds the number of built tags of a group to the summary.
    template <typename TMapObject>
    auto AppendCount(std::string& summary, std::int32_t count) -> void
    {
        if (!summary.empty()) summary += " | ";

        summary += std::format("{}: {}", FourCC(GroupDescriptor<TMapObject>::Magic), count);
    }

    // Finds the store of a catalog that holds objects of the given type.
    template <typename TObject, typename TCatalog>
    auto StoreOf(TCatalog& catalog) -> TagStore<TObject>&
    {
        TagStore<TObject>* found{};

        catalog.ForEach([&found](auto& store) {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(store)>, TagStore<TObject>>)
            {
                found = &store;
            }
        });

        return *found;
    }

    // Builds every tag of the group the builder reads.
    template <typename TBuilder, typename TCatalog, typename TDefinitions>
    auto BuildGroup(TBuilder& builder, TCatalog& tagCatalog, TDefinitions& definitions,
        std::string& summary) -> std::int32_t
    {
        using Traits = BuildTraits<decltype(&TBuilder::Build)>;

        const auto& tags = StoreOf<typename Traits::MapObject>(tagCatalog).All();
        auto& built = StoreOf<typename Traits::Definition>(definitions);

        for (const auto& [tagName, object] : tags)
        {
            built.Add(tagName, builder.Build(object));
        }

        const std::int32_t count{ static_cast<std::int32_t>(tags.size()) };

        AppendCount<typename Traits::MapObject>(summary, count);

        return count;
    }
}

namespace Resolved::Definitions::System
{
    auto DefinitionsBuilder::BuildForMap() -> void
    {
        std::string summary{};

        std::apply([&](auto&... builder) {
            (BuildGroup(builder, m_TagCatalog, m_DefinitionsStore, summary), ...);
        }, m_Builders);

        AppendCount<SbspObject>(summary, this->BuildSbsps());

        m_DefinitionsStore.Freeze();

        m_LogsService.Message("[DefinitionsBuilder] INFO: Definitions built. {}.", summary);
    }

    auto DefinitionsBuilder::BuildSbsps() -> std::int32_t
    {
        const auto& sbsps = m_TagCatalog.Sbsp.All();
        if (sbsps.empty()) return 0;

        std::vector<std::string> tagNames{};
        tagNames.reserve(sbsps.size());

        for (const auto& [tagName, sbsp] : sbsps)
        {
            tagNames.push_back(tagName);
        }

        std::vector<SbspGeometry> rendered = m_GeometryLoaderService.ReadRenderGeometry(tagNames);

        std::unordered_map<std::string, std::vector<Triangle>> renderByName{};
        renderByName.reserve(rendered.size());

        for (SbspGeometry& render : rendered)
        {
            renderByName.emplace(render.TagName, std::move(render.RenderGeometry));
        }

        if (renderByName.empty())
        {
            m_LogsService.Message("[DefinitionsBuilder] ERROR:"
                " Failed to read render geometry.");
        }
        else if (renderByName.size() < tagNames.size())
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Render geometry read for {} of {} sbsp.",
                static_cast<int>(renderByName.size()),
                static_cast<int>(tagNames.size()));
        }

        std::int32_t built{};

        for (const auto& [tagName, sbsp] : sbsps)
        {
            std::vector<Triangle> renderGeometry{};

            const auto it = renderByName.find(tagName);
            if (it != renderByName.end())
            {
                renderGeometry = std::move(it->second);
            }

            m_DefinitionsStore.Sbsp.Add(tagName, m_SbspBuilder.Build(sbsp, std::move(renderGeometry)));
            ++built;
        }

        return built;
    }

    auto DefinitionsBuilder::Cleanup() -> void
    {
        m_DefinitionsStore.Cleanup();

        m_LogsService.Message("[DefinitionsBuilder] INFO: Cleanup completed.");
    }
}