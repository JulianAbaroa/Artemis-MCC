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
        using ResolvedVitality = Resolved::Vitality::Type::Vitality;

        using LogsService = Service::Logs::System::LogsService;
        template <typename TObject> using CollStore = Map::Tag::State::Coll::CollStore<TObject>;
        template <typename TObject> using HlmtStore = Map::Tag::State::Hlmt::HlmtStore<TObject>;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

        const std::uint32_t k_FlagKillsObject = (1u << 0);       // "Kills Object"
        const std::uint32_t k_FlagKillsObjectNoSolo = (1u << 10);// "Kills Object (No Player Solo)"
        const std::uint32_t k_FlagHeadshot = (1u << 4);          // "Headshot"

    public:
        VitalityBuilder(LogsService& logsService, CollStore<CollObject>& collStore,
            HlmtStore<HlmtObject>& hlmtStore, VitalityStore& vitalityStore) :
            m_LogsService(logsService), m_CollStore(collStore), 
            m_HlmtStore(hlmtStore), m_VitalityStore(vitalityStore) {}
        ~VitalityBuilder() = default;

        auto BuildForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        CollStore<CollObject>& m_CollStore;
        HlmtStore<HlmtObject>& m_HlmtStore;
        VitalityStore& m_VitalityStore;

        auto BuildLayout(const HlmtObject& hlmt, const CollObject* coll) const -> ResolvedVitality;
    };
}