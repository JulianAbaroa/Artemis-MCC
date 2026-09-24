export module Resolved.World.System;

export import :Coll;
export import :Mode;
export import :Phmo;
export import :RegionStates;
export import :Sbsp;
export import :SbspSeamLinker;

import Service.Logs.System;
import Map.Reader.State;
import Map.Reader.System;
import Map.Tag.Type;
import Map.Tag.State;
import Map.Tag.System;
import Resolved.World.Type;
import Resolved.World.State;
import std;

export namespace Resolved::World::System
{
    class WorldBuilder
    {
    private:
        using CollObject = Map::Tag::Type::Coll::Object::CollObject;
        using ModeObject = Map::Tag::Type::Mode::Object::ModeObject;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;
        using ResolvedSbsp = Resolved::World::Type::Sbsp::Sbsp;

        using LogsService = Service::Logs::System::LogsService;
		using TagIndexStore = Map::Reader::State::TagIndexStore;
        using TagCatalog = Map::Tag::State::TagCatalog;
        template <typename TObject> using TagStore = Map::Reader::State::TagStore<TObject>;
        using GeometryLoaderService = Map::Reader::System::GeometryLoaderService;
        using WorldStore = Resolved::World::State::WorldStore;
		using TagResolverService = Map::Reader::System::TagResolverService;
        using SbspBuilder = Resolved::World::System::SbspBuilder;
        using SbspSeamLinker = Resolved::World::System::SbspSeamLinker;
        using CollBuilder = Resolved::World::System::CollBuilder;
        using ModeBuilder = Resolved::World::System::ModeBuilder;
        using RegionStatesBuilder = Resolved::World::System::RegionStatesBuilder;

    public:
        WorldBuilder(LogsService& logsService, TagIndexStore& m_TagIndexStore, 
            TagCatalog& tagCatalog, WorldStore& worldStore, 
            TagResolverService& tagResolverService,
			GeometryLoaderService& geometryLoaderService,
            SbspSeamLinker& sbspSeamLinker, SbspBuilder& sbspBuilder,
            CollBuilder& collBuilder, ModeBuilder& modeBuilder,
            RegionStatesBuilder& regionStatesBuilder) : m_LogsService(logsService), 
            m_TagIndexStore(m_TagIndexStore), m_TagCatalog(tagCatalog), 
            m_WorldStore(worldStore), m_TagResolverService(tagResolverService),
            m_GeometryLoaderService(geometryLoaderService),
            m_SbspSeamLinker(sbspSeamLinker), m_SbspBuilder(sbspBuilder),
            m_CollBuilder(collBuilder), m_ModeBuilder(modeBuilder),
            m_RegionStatesBuilder(regionStatesBuilder) {}
        ~WorldBuilder() = default;

        auto BuildForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        TagIndexStore& m_TagIndexStore;
        TagCatalog& m_TagCatalog;
        WorldStore& m_WorldStore;
        TagResolverService& m_TagResolverService;
        GeometryLoaderService& m_GeometryLoaderService;
        SbspSeamLinker& m_SbspSeamLinker;
        SbspBuilder& m_SbspBuilder;
        CollBuilder& m_CollBuilder;
        ModeBuilder& m_ModeBuilder;
        RegionStatesBuilder& m_RegionStatesBuilder;

        auto BuildColl(const std::string& tagName) -> bool;
        auto BuildMode(const std::string& tagName) -> bool;

        auto BuildSbsp(const std::string& tagName,
            std::vector<ResolvedSbsp>& geometries, 
            std::vector<const SbspObject*>& sbspObjects) -> bool;

        auto LinkObjectColls() -> void;

        template <typename TObject>
        auto LinkObjectFamily(const TagStore<TObject>& state,
            std::unordered_set<std::string>& builtRegionStates) -> void;

        auto BuildRegionStates(const std::string& hlmtName,
            const HlmtObject& hlmt, const std::string& collName) -> void;
    };
}