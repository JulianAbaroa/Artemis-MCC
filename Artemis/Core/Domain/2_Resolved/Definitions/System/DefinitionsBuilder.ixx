export module Resolved.Definitions.System;

// Damage
export import :Jpt;

// Level
export import :Sbsp;
export import :Scnr;

// Model
export import :Coll;
export import :Ctrl;
export import :Hlmt;

// Object
export import :Object;
export import :Bipd;
export import :Bloc;
export import :Eqip;
export import :Mach;
export import :Mode;
export import :Proj;
export import :Scen;
export import :Vehi;
export import :Weap;

import Service.Logs.System;
import Map.Reader.State;
import Map.Reader.System;
import Map.Tag.Type;
import Map.Tag.State;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import std;

export namespace Resolved::Definitions::System
{
    class DefinitionsBuilder
    {
    private:
        using BipdObject = Map::Tag::Type::Bipd::Object::BipdObject;
        using BlocObject = Map::Tag::Type::Bloc::Object::BlocObject;
        using CollObject = Map::Tag::Type::Coll::Object::CollObject;
        using CtrlObject = Map::Tag::Type::Ctrl::Object::CtrlObject;
        using EqipObject = Map::Tag::Type::Eqip::Object::EqipObject;
        using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;
        using JptObject = Map::Tag::Type::Jpt::Object::JptObject;
        using MachObject = Map::Tag::Type::Mach::Object::MachObject;
        using ModeObject = Map::Tag::Type::Mode::Object::ModeObject;
        using ProjObject = Map::Tag::Type::Proj::Object::ProjObject;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using ScenObject = Map::Tag::Type::Scen::Object::ScenObject;
        using ScnrObject = Map::Tag::Type::Scnr::Object::ScnrObject;
        using VehiObject = Map::Tag::Type::Vehi::Object::VehiObject;
        using WeapObject = Map::Tag::Type::Weap::Object::WeapObject;

        using LogsService = Service::Logs::System::LogsService;
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using TagCatalog = Map::Tag::State::TagCatalog;
        using GeometryLoaderService = Map::Reader::System::GeometryLoaderService;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using MachBuilder = Resolved::Definitions::System::MachBuilder;
        using ModeBuilder = Resolved::Definitions::System::ModeBuilder;
        using ProjBuilder = Resolved::Definitions::System::ProjBuilder;
        using SbspBuilder = Resolved::Definitions::System::SbspBuilder;
        using ScenBuilder = Resolved::Definitions::System::ScenBuilder;
        using ScnrBuilder = Resolved::Definitions::System::ScnrBuilder;
        using VehiBuilder = Resolved::Definitions::System::VehiBuilder;
        using WeapBuilder = Resolved::Definitions::System::WeapBuilder;

    public:
        DefinitionsBuilder(LogsService& logsService, TagIndexStore& m_TagIndexStore,
            TagCatalog& tagCatalog, GeometryLoaderService& geometryLoaderService,
            DefinitionsStore& definitionsStore, BipdBuilder& bipdBuilder,
            BlocBuilder& blocBuilder, CollBuilder& collBuilder, CtrlBuilder& ctrlBuilder,
            EqipBuilder& eqipBuilder, HlmtBuilder& hlmtBuilder, JptBuilder& jptBuilder, MachBuilder& machBuilder,
            ModeBuilder& modeBuilder, ProjBuilder& ProjBuilder, SbspBuilder& sbspBuilder,
            ScenBuilder& scenBuilder, ScnrBuilder& scnrBuilder, VehiBuilder& VehiBuilder, WeapBuilder& WeapBuilder) :
            m_LogsService(logsService), m_TagIndexStore(m_TagIndexStore), m_TagCatalog(tagCatalog),
            m_GeometryLoaderService(geometryLoaderService), m_DefinitionsStore(definitionsStore), 
            m_BipdBuilder(bipdBuilder), m_BlocBuilder(blocBuilder), m_CollBuilder(collBuilder), 
            m_CtrlBuilder(ctrlBuilder), m_EqipBuilder(eqipBuilder), m_HlmtBuilder(hlmtBuilder), 
            m_JptBuilder(jptBuilder), m_MachBuilder(machBuilder), m_ModeBuilder(modeBuilder), 
            m_ProjBuilder(ProjBuilder), m_SbspBuilder(sbspBuilder), 
            m_ScenBuilder(scenBuilder), m_ScnrBuilder(scnrBuilder), m_VehiBuilder(VehiBuilder), 
            m_WeapBuilder(WeapBuilder) {}
        ~DefinitionsBuilder() = default;

        auto BuildForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        TagIndexStore& m_TagIndexStore;
        TagCatalog& m_TagCatalog;
        GeometryLoaderService& m_GeometryLoaderService;
        DefinitionsStore& m_DefinitionsStore;
        BipdBuilder& m_BipdBuilder;
        BlocBuilder& m_BlocBuilder;
        CollBuilder& m_CollBuilder;
        CtrlBuilder& m_CtrlBuilder;
        EqipBuilder& m_EqipBuilder;
        HlmtBuilder& m_HlmtBuilder;
        JptBuilder& m_JptBuilder;
        MachBuilder& m_MachBuilder;
        ModeBuilder& m_ModeBuilder;
        ProjBuilder& m_ProjBuilder;
        SbspBuilder& m_SbspBuilder;
        ScenBuilder& m_ScenBuilder;
        ScnrBuilder& m_ScnrBuilder;
        VehiBuilder& m_VehiBuilder;
        WeapBuilder& m_WeapBuilder;

        auto BuildBipd(const std::string& tagName) -> bool;
        auto BuildBloc(const std::string& tagName) -> bool;
        auto BuildColl(const std::string& tagName) -> bool;
        auto BuildCtrl(const std::string& tagName) -> bool;
        auto BuildEqip(const std::string& tagName) -> bool;
        auto BuildHlmt(const std::string& tagName) -> bool;
        auto BuildJpt(const std::string& tagName) -> bool;
        auto BuildMach(const std::string& tagName) -> bool;
        auto BuildMode(const std::string& tagName) -> bool;
        auto BuildProj(const std::string& tagName) -> bool;
        auto BuildSbsps(std::vector<std::string>& tagNames) -> std::int32_t;
        auto BuildScen(const std::string& tagName) -> bool;
        auto BuildScnr(const std::string& tagName) -> bool;
        auto BuildVehi(const std::string& tagName) -> bool;
        auto BuildWeap(const std::string& tagName) -> bool;
    };
}