export module Resolved.Vitality.System;

import Service.Logs.System;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Resolved.World.Type;
import Resolved.World.State;
import Resolved.Vitality.Type;
import Resolved.Vitality.State;
import std;

export namespace Resolved::Vitality::System
{
    class VitalityBuilder
    {
    private:
        using ResolvedHlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using ResolvedModelLink = Resolved::World::Type::ModelLink::ModelLink;
        using AimAnchor = Resolved::World::Type::ModelLink::Anchor;
        using ResolvedVitality = Resolved::Vitality::Type::Vitality::Vitality;
        using Section = Resolved::Vitality::Type::Vitality::Section;

        using LogsService = Service::Logs::System::LogsService;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using WorldStore = Resolved::World::State::WorldStore;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

    public:
        VitalityBuilder(LogsService& logsService, DefinitionsStore& definitionsStore,
            WorldStore& worldStore, VitalityStore& vitalityStore) : m_LogsService(logsService),
            m_DefinitionsStore(definitionsStore), m_WorldStore(worldStore),
            m_VitalityStore(vitalityStore) {}
        ~VitalityBuilder() = default;

        auto BuildForMap() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        DefinitionsStore& m_DefinitionsStore;
        WorldStore& m_WorldStore;
        VitalityStore& m_VitalityStore;

        auto BuildLayout(const ResolvedHlmt& hlmt, const ResolvedModelLink* link) const -> ResolvedVitality;

        // Anchor cascade: ModelTarget of the section -> coll region of the section ->
        // Headshot-lock ModelTarget (Headshot sections) -> object center (non-shield sections)
        auto ResolveAnchor(const Section& section, const ResolvedModelLink& link) const -> AimAnchor;
    };
}
