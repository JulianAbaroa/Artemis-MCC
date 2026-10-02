export module Resolved.Vitality.System;

import Service.Logs.System;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Resolved.Vitality.Type;
import Resolved.Vitality.State;
import std;

export namespace Resolved::Vitality::System
{
    class VitalityBuilder
    {
    private:
        using ResolvedColl = Resolved::Definitions::Type::Coll::Coll;
        using ResolvedHlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using ResolvedVitality = Resolved::Vitality::Type::Vitality::Vitality;

        using LogsService = Service::Logs::System::LogsService;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

    public:
        VitalityBuilder(LogsService& logsService, DefinitionsStore& definitionsStore,
            VitalityStore& vitalityStore) : m_LogsService(logsService), 
            m_DefinitionsStore(definitionsStore), m_VitalityStore(vitalityStore) { }
        ~VitalityBuilder() = default;

        auto BuildForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        DefinitionsStore& m_DefinitionsStore;
        VitalityStore& m_VitalityStore;

        auto BuildLayout(const ResolvedHlmt& hlmt, const ResolvedColl* coll) const -> ResolvedVitality;
    };
}