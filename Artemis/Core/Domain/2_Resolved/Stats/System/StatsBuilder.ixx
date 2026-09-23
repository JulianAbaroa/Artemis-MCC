export module Resolved.Stats.System;

export import :Proj;
export import :Vehi;
export import :Weap;

import Service.Logs.System;
import Map.Reader.State;
import Map.Tag.Type;
import Map.Tag.State;
import Resolved.Stats.Type;
import Resolved.Stats.State;
import std;

export namespace Resolved::Stats::System
{
    class StatsBuilder
    {
    private:
        using ProjObject = Map::Tag::Type::Proj::Object::ProjObject;
        using VehiObject = Map::Tag::Type::Vehi::Object::VehiObject;
        using WeapObject = Map::Tag::Type::Weap::Object::WeapObject;

        using LogsService = Service::Logs::System::LogsService;
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        template <typename TObject> using ProjStore = Map::Tag::State::Proj::ProjStore<TObject>;
        template <typename TObject> using VehiStore = Map::Tag::State::Vehi::VehiStore<TObject>;
        template <typename TObject> using WeapStore = Map::Tag::State::Weap::WeapStore<TObject>;
        using StatsStore = Resolved::Stats::State::StatsStore;
        using ProjBuilder = Resolved::Stats::System::ProjBuilder;
        using VehiBuilder = Resolved::Stats::System::VehiBuilder;
        using WeapBuilder = Resolved::Stats::System::WeapBuilder;

    public:
        StatsBuilder(LogsService& logsService, TagIndexStore& m_TagIndexStore,
            ProjStore<ProjObject>& projStore, VehiStore<VehiObject>& vehiStore,
            WeapStore<WeapObject>& weapStore, StatsStore& StatsStore,
            ProjBuilder& ProjBuilder, VehiBuilder& VehiBuilder, 
            WeapBuilder& WeapBuilder) : m_LogsService(logsService), 
            m_TagIndexStore(m_TagIndexStore), m_ProjStore(projStore), 
            m_VehiStore(vehiStore), m_WeapStore(weapStore), 
            m_StatsStore(StatsStore), m_ProjBuilder(ProjBuilder), 
            m_VehiBuilder(VehiBuilder), m_WeapBuilder(WeapBuilder) {}
        ~StatsBuilder() = default;

        auto BuildForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        TagIndexStore& m_TagIndexStore;
        ProjStore<ProjObject>& m_ProjStore;
        VehiStore<VehiObject>& m_VehiStore;
        WeapStore<WeapObject>& m_WeapStore;
        StatsStore& m_StatsStore;
        ProjBuilder& m_ProjBuilder;
        VehiBuilder& m_VehiBuilder;
        WeapBuilder& m_WeapBuilder;

        auto BuildVehi(const std::string& tagName) -> bool;
        auto BuildWeap(const std::string& tagName) -> bool;
        auto BuildProj(const std::string& tagName) -> bool;
    };
}