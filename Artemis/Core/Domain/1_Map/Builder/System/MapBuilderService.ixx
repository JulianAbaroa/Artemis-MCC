export module Map.Builder.System;

import Service.Logs.System;
import Map.Reader.State;
import Map.Reader.System;
import Map.Tag.Type;
import Map.Tag.State;
import Map.Tag.System;
import std;

export namespace Map::Builder::System
{
    class MapBuilderService
    {
    private:
        using BipdObject = Map::Tag::Type::Bipd::Object::BipdObject;
        using BlocObject = Map::Tag::Type::Bloc::Object::BlocObject;
        using CollObject = Map::Tag::Type::Coll::Object::CollObject;
        using CtrlObject = Map::Tag::Type::Ctrl::Object::CtrlObject;
        using EqipObject = Map::Tag::Type::Eqip::Object::EqipObject;
        using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;
        using LbspObject = Map::Tag::Type::Lbsp::Object::LbspObject;
        using MachObject = Map::Tag::Type::Mach::Object::MachObject;
        using ModeObject = Map::Tag::Type::Mode::Object::ModeObject;
        using PhmoObject = Map::Tag::Type::Phmo::Object::PhmoObject;
        using PlayObject = Map::Tag::Type::Play::Object::PlayObject;
        using ProjObject = Map::Tag::Type::Proj::Object::ProjObject;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using ScenObject = Map::Tag::Type::Scen::Object::ScenObject;
        using ScnrObject = Map::Tag::Type::Scnr::Object::ScnrObject;
        using SldtObject = Map::Tag::Type::Sldt::Object::SldtObject;
        using VehiObject = Map::Tag::Type::Vehi::Object::VehiObject;
        using WeapObject = Map::Tag::Type::Weap::Object::WeapObject;
        using ZoneObject = Map::Tag::Type::Zone::Object::ZoneObject;

        using LogsService = Service::Logs::System::LogsService;
        template <typename TObject> using BipdStore = Map::Tag::State::Bipd::BipdStore<TObject>;
        template <typename TObject> using BlocStore = Map::Tag::State::Bloc::BlocStore<TObject>;
        template <typename TObject> using CollStore = Map::Tag::State::Coll::CollStore<TObject>;
        template <typename TObject> using CtrlStore = Map::Tag::State::Ctrl::CtrlStore<TObject>;
        template <typename TObject> using EqipStore = Map::Tag::State::Eqip::EqipStore<TObject>;
        template <typename TObject> using HlmtStore = Map::Tag::State::Hlmt::HlmtStore<TObject>;
        template <typename TObject> using LbspStore = Map::Tag::State::Lbsp::LbspStore<TObject>;
        template <typename TObject> using MachStore = Map::Tag::State::Mach::MachStore<TObject>;
        template <typename TObject> using ModeStore = Map::Tag::State::Mode::ModeStore<TObject>;
        template <typename TObject> using PhmoStore = Map::Tag::State::Phmo::PhmoStore<TObject>;
        template <typename TObject> using PlayStore = Map::Tag::State::Play::PlayStore<TObject>;
        template <typename TObject> using ProjStore = Map::Tag::State::Proj::ProjStore<TObject>;
        template <typename TObject> using SbspStore = Map::Tag::State::Sbsp::SbspStore<TObject>;
        template <typename TObject> using ScenStore = Map::Tag::State::Scen::ScenStore<TObject>;
        template <typename TObject> using ScnrStore = Map::Tag::State::Scnr::ScnrStore<TObject>;
        template <typename TObject> using SldtStore = Map::Tag::State::Sldt::SldtStore<TObject>;
        template <typename TObject> using VehiStore = Map::Tag::State::Vehi::VehiStore<TObject>;
        template <typename TObject> using WeapStore = Map::Tag::State::Weap::WeapStore<TObject>;
        template <typename TObject> using ZoneStore = Map::Tag::State::Zone::ZoneStore<TObject>;
        using FileStore = Map::Reader::State::FileStore;
        using TagIndexStore = Map::Reader::State::TagIndexStore;
        using FileLocatorService = Map::Reader::System::FileLocatorService;
        using TagResolverService = Map::Reader::System::TagResolverService;
        using TagGroupService = Map::Reader::System::TagGroupService;

    public:
        MapBuilderService(LogsService& logsService, BipdStore<BipdObject>& bipdStore,
            BlocStore<BlocObject>& blocStore, CollStore<CollObject>& collStore,
            CtrlStore<CtrlObject>& ctrlStore, EqipStore<EqipObject>& eqipStore,
            HlmtStore<HlmtObject>& hlmtStore, LbspStore<LbspObject>& lbspStore,
            MachStore<MachObject>& machStore, ModeStore<ModeObject>& modeStore,
            PhmoStore<PhmoObject>& phmoStore, PlayStore<PlayObject>& playStore,
            ProjStore<ProjObject>& projStore, SbspStore<SbspObject>& sbspStore,
            ScenStore<ScenObject>& scenStore, ScnrStore<ScnrObject>& scnrStore,
            SldtStore<SldtObject>& sldtStore, VehiStore<VehiObject>& vehiStore,
            WeapStore<WeapObject>& weapStore, ZoneStore<ZoneObject>& zoneStore,
            FileStore& fileStore, TagIndexStore& m_TagIndexStore,
            FileLocatorService& fileLocatorService, 
            TagResolverService& tagResolverService) :
            m_LogsService(logsService), m_BipdStore(bipdStore), m_BlocStore(blocStore), 
            m_CollStore(collStore), m_CtrlStore(ctrlStore), m_EqipStore(eqipStore),
            m_HlmtStore(hlmtStore), m_LbspStore(lbspStore), m_MachStore(machStore),
            m_ModeStore(modeStore), m_PhmoStore(phmoStore), m_PlayStore(playStore),
            m_ProjStore(projStore), m_SbspStore(sbspStore), m_ScenStore(scenStore),
            m_ScnrStore(scnrStore), m_SldtStore(sldtStore), m_VehiStore(vehiStore),
            m_WeapStore(weapStore), m_ZoneStore(zoneStore), m_FileStore(fileStore),
            m_TagIndexStore(m_TagIndexStore), m_FileLocatorService(fileLocatorService),
            m_TagResolverService(tagResolverService) {}
        ~MapBuilderService() = default;

        auto LoadForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        BipdStore<BipdObject>& m_BipdStore;
        BlocStore<BlocObject>& m_BlocStore;
        CollStore<CollObject>& m_CollStore;
        CtrlStore<CtrlObject>& m_CtrlStore;
        EqipStore<EqipObject>& m_EqipStore;
        HlmtStore<HlmtObject>& m_HlmtStore;
        LbspStore<LbspObject>& m_LbspStore;
        MachStore<MachObject>& m_MachStore;
        ModeStore<ModeObject>& m_ModeStore;
        PhmoStore<PhmoObject>& m_PhmoStore;
        PlayStore<PlayObject>& m_PlayStore;
        ProjStore<ProjObject>& m_ProjStore;
        SbspStore<SbspObject>& m_SbspStore;
        ScenStore<ScenObject>& m_ScenStore;
        ScnrStore<ScnrObject>& m_ScnrStore;
        SldtStore<SldtObject>& m_SldtStore;
        VehiStore<VehiObject>& m_VehiStore;
        WeapStore<WeapObject>& m_WeapStore;
        ZoneStore<ZoneObject>& m_ZoneStore;
        FileStore& m_FileStore;
        TagIndexStore& m_TagIndexStore;
        FileLocatorService& m_FileLocatorService;
        TagResolverService& m_TagResolverService;

        auto LoadTags(std::ifstream& file, TagGroupService& reader) -> void;
    };
}