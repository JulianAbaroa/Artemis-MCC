export module Resolved.Vitality.System;

import Service.Logs.System;
import Map.Tag.Type;
import Map.Tag.State;
import Resolved.Vitality.Type;
import Resolved.Vitality.State;
import std;

export namespace Resolved::Vitality::System
{
    class VitalityBuilder
    {
    private:
        using CollObject = Map::Tag::Type::Coll::Object::CollObject;
        using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;
        using ResolvedVitality = Resolved::Vitality::Type::Vitality::Vitality;

        using LogsService = Service::Logs::System::LogsService;
        using TagCatalog = Map::Tag::State::TagCatalog;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

    public:
        VitalityBuilder(LogsService& logsService, TagCatalog& tagCatalog, 
            VitalityStore& vitalityStore) : m_LogsService(logsService), 
            m_TagCatalog(tagCatalog), m_VitalityStore(vitalityStore) { }
        ~VitalityBuilder() = default;

        auto BuildForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        TagCatalog& m_TagCatalog;
        VitalityStore& m_VitalityStore;

        auto BuildLayout(const HlmtObject& hlmt, const CollObject* coll) const -> ResolvedVitality;
    };
}