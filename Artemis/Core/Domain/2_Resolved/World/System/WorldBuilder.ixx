export module Resolved.World.System;

export import :RegionStates;
export import :ModelLink;
export import :Raycast;

import Service.Logs.System;
import Map.Reader.State;
import Map.Reader.System;
import Map.Tag.Type;
import Map.Tag.State;
import Map.Tag.System;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Resolved.World.Type;
import Resolved.World.State;
import std;

export namespace Resolved::World::System
{
    class WorldBuilder
    {
    private:
        using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;

        using LogsService = Service::Logs::System::LogsService;
        using TagCatalog = Map::Tag::State::TagCatalog;
        template <typename TObject> using TagStore = Map::Reader::State::TagStore<TObject>;
        using WorldStore = Resolved::World::State::WorldStore;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using TagResolverService = Map::Reader::System::TagResolverService;
        using RegionStatesBuilder = Resolved::World::System::RegionStatesBuilder;
        using ModelLinkBuilder = Resolved::World::System::ModelLinkBuilder;

    public:
        WorldBuilder(LogsService& logsService, TagCatalog& tagCatalog,
            WorldStore& worldStore, DefinitionsStore& definitionsStore,
            TagResolverService& tagResolverService,
            RegionStatesBuilder& regionStatesBuilder,
            ModelLinkBuilder& modelLinkBuilder) : m_LogsService(logsService),
            m_TagCatalog(tagCatalog), m_WorldStore(worldStore),
            m_DefinitionsStore(definitionsStore),
            m_TagResolverService(tagResolverService),
            m_RegionStatesBuilder(regionStatesBuilder),
            m_ModelLinkBuilder(modelLinkBuilder) {}
        ~WorldBuilder() = default;

        auto BuildForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        TagCatalog& m_TagCatalog;
        WorldStore& m_WorldStore;
        DefinitionsStore& m_DefinitionsStore;
        TagResolverService& m_TagResolverService;
        RegionStatesBuilder& m_RegionStatesBuilder;
        ModelLinkBuilder& m_ModelLinkBuilder;

        auto LinkObjectColls() -> void;

        auto BuildModelLinks() -> void;

        template <typename TObject>
        auto LinkObjectFamily(const TagStore<TObject>& state,
            std::unordered_set<std::string>& builtRegionStates) -> void;

        auto BuildRegionStates(const std::string& hlmtName,
            const HlmtObject& hlmt, const std::string& collName) -> void;
    };
}