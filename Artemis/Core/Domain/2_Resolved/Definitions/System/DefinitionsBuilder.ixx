export module Resolved.Definitions.System;

// Damage
export import :Jpt;

// Level
export import :Sbsp;
export import :Scnr;

// Model
export import :Coll;
export import :Hlmt;
export import :Mode;

// Object
export import :Object;
export import :Bipd;
export import :Bloc;
export import :Ctrl;
export import :Eqip;
export import :Mach;
export import :Proj;
export import :Scen;
export import :Vehi;
export import :Weap;

import Service.Logs.System;
import Map.Reader.System;
import Map.Tag.State;
import Map.Tag.System;
import Resolved.Definitions.State;
import std;

export namespace Resolved::Definitions::System
{
    // Builds the definitions of every tag the map layer loaded.
    // note: Every group goes through the same loop. Each builder says which map object it reads and
    // which definition it returns through its Build signature. Sbsp is apart because it needs geometry.
    class DefinitionsBuilder
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using GeometryLoaderService = Map::Reader::System::GeometryLoaderService;
        using TagResolverService = Map::Reader::System::TagResolverService;
        using TagCatalog = Map::Tag::State::TagCatalog;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;

    public:
        DefinitionsBuilder(LogsService& logsService, TagCatalog& tagCatalog,
            GeometryLoaderService& geometryLoaderService, DefinitionsStore& definitionsStore,
            TagResolverService& tagResolverService) :
            m_LogsService(logsService), m_TagCatalog(tagCatalog),
            m_GeometryLoaderService(geometryLoaderService), m_DefinitionsStore(definitionsStore),
            m_ObjectBuilder(tagResolverService),
            m_Builders{ BipdBuilder{ m_ObjectBuilder }, BlocBuilder{ m_ObjectBuilder }, CollBuilder{},
                CtrlBuilder{ m_ObjectBuilder }, EqipBuilder{ m_ObjectBuilder }, HlmtBuilder{ tagResolverService },
                JptBuilder{}, MachBuilder{ m_ObjectBuilder }, ModeBuilder{}, ProjBuilder{ m_ObjectBuilder },
                ScenBuilder{ m_ObjectBuilder }, ScnrBuilder{}, VehiBuilder{ m_ObjectBuilder },
                WeapBuilder{ m_ObjectBuilder } } {}
        ~DefinitionsBuilder() = default;

        // Builds the definitions of the loaded map and freezes the store.
        // note: Runs after the map layer froze the tag catalog.
        auto BuildForMap() -> void;

        // Removes every definition.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        TagCatalog& m_TagCatalog;
        GeometryLoaderService& m_GeometryLoaderService;
        DefinitionsStore& m_DefinitionsStore;

        ObjectBuilder m_ObjectBuilder;
        std::tuple<BipdBuilder, BlocBuilder, CollBuilder, CtrlBuilder, EqipBuilder, HlmtBuilder,
            JptBuilder, MachBuilder, ModeBuilder, ProjBuilder, ScenBuilder, ScnrBuilder,
            VehiBuilder, WeapBuilder> m_Builders;
        SbspBuilder m_SbspBuilder{};

        // return: Number of sbsp built.
        auto BuildSbsps() -> std::int32_t;
    };
}