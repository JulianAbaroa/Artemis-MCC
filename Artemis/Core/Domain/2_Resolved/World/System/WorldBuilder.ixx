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
        using BipdObject = Map::Tag::Type::Bipd::Object::BipdObject;
        using BlocObject = Map::Tag::Type::Bloc::Object::BlocObject;
        using CollObject = Map::Tag::Type::Coll::Object::CollObject;
        using CtrlObject = Map::Tag::Type::Ctrl::Object::CtrlObject;
        using EqipObject = Map::Tag::Type::Eqip::Object::EqipObject;
        using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;
        using MachObject = Map::Tag::Type::Mach::Object::MachObject;
        using ModeObject = Map::Tag::Type::Mode::Object::ModeObject;
        using ScenObject = Map::Tag::Type::Scen::Object::ScenObject;
        using VehiObject = Map::Tag::Type::Vehi::Object::VehiObject;
        using WeapObject = Map::Tag::Type::Weap::Object::WeapObject;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using ResolvedSbsp = Resolved::World::Type::Sbsp::Sbsp;

        using LogsService = Service::Logs::System::LogsService;
		using TagIndexStore = Map::Reader::State::TagIndexStore;
        template <typename TObject> using BipdStore = Map::Tag::State::Bipd::BipdStore<TObject>;
        template <typename TObject> using BlocStore = Map::Tag::State::Bloc::BlocStore<TObject>;
        template <typename TObject> using CollStore = Map::Tag::State::Coll::CollStore<TObject>;
        template <typename TObject> using CtrlStore = Map::Tag::State::Ctrl::CtrlStore<TObject>;
        template <typename TObject> using EqipStore = Map::Tag::State::Eqip::EqipStore<TObject>;
        template <typename TObject> using HlmtStore = Map::Tag::State::Hlmt::HlmtStore<TObject>;
        template <typename TObject> using MachStore = Map::Tag::State::Mach::MachStore<TObject>;
        template <typename TObject> using ModeStore = Map::Tag::State::Mode::ModeStore<TObject>;
        template <typename TObject> using SbspStore = Map::Tag::State::Sbsp::SbspStore<TObject>;
        template <typename TObject> using ScenStore = Map::Tag::State::Scen::ScenStore<TObject>;
        template <typename TObject> using VehiStore = Map::Tag::State::Vehi::VehiStore<TObject>;
        template <typename TObject> using WeapStore = Map::Tag::State::Weap::WeapStore<TObject>;
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
            BipdStore<BipdObject>& bipdStore, BlocStore<BlocObject>& blocStore,
            CollStore<CollObject>& collStore, CtrlStore<CtrlObject>& ctrlStore,
            EqipStore<EqipObject>& eqipStore, HlmtStore<HlmtObject>& hlmtStore,
            MachStore<MachObject>& machStore, ModeStore<ModeObject>& modeStore,
            SbspStore<SbspObject>& sbspStore, ScenStore<ScenObject>& scenStore,
            VehiStore<VehiObject>& vehiStore, WeapStore<WeapObject>& weapStore,
            WorldStore& worldStore, TagResolverService& tagResolverService,
			GeometryLoaderService& geometryLoaderService,
            SbspSeamLinker& sbspSeamLinker, SbspBuilder& sbspBuilder,
            CollBuilder& collBuilder, ModeBuilder& modeBuilder,
            RegionStatesBuilder& regionStatesBuilder) : m_LogsService(logsService), 
            m_TagIndexStore(m_TagIndexStore), m_BipdStore(bipdStore), 
            m_BlocStore(blocStore), m_CollStore(collStore), m_CtrlStore(ctrlStore), 
            m_EqipStore(eqipStore), m_HlmtStore(hlmtStore), m_MachStore(machStore), 
            m_ModeStore(modeStore), m_SbspStore(sbspStore), m_ScenStore(scenStore), 
            m_VehiStore(vehiStore), m_WeapStore(weapStore), m_WorldStore(worldStore), 
            m_TagResolverService(tagResolverService),
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
        BipdStore<BipdObject>& m_BipdStore;
        BlocStore<BlocObject>& m_BlocStore;
        CollStore<CollObject>& m_CollStore;
        CtrlStore<CtrlObject>& m_CtrlStore;
        EqipStore<EqipObject>& m_EqipStore;
        HlmtStore<HlmtObject>& m_HlmtStore;
        MachStore<MachObject>& m_MachStore;
        ModeStore<ModeObject>& m_ModeStore;
        SbspStore<SbspObject>& m_SbspStore;
        ScenStore<ScenObject>& m_ScenStore;
        VehiStore<VehiObject>& m_VehiStore;
        WeapStore<WeapObject>& m_WeapStore;
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