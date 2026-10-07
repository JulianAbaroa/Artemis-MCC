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
    // Builds the vitality layout of every model that has damage sections.
    class VitalityBuilder
    {
    private:
        using Hlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using ModelLink = Resolved::World::Type::ModelLink::ModelLink;
        using Anchor = Resolved::World::Type::ModelLink::Anchor;
        using Vitality = Resolved::Vitality::Type::Vitality::Vitality;
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

        // Builds the layouts from the definitions and the model links, then freezes the store.
        // note: Runs after the definitions and the world are built.
        auto BuildForMap() -> void;

        // Removes every layout.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        DefinitionsStore& m_DefinitionsStore;
        WorldStore& m_WorldStore;
        VitalityStore& m_VitalityStore;

        // Reads the roles, transfers and anchors of every damage section of the model.
        auto BuildLayout(const Hlmt& hlmt, const ModelLink* link) const -> Vitality;

        // Finds where to aim for a section.
        // note: Cascade: model target of the section, coll region of the section,
        // headshot lock-on model target (headshot sections), object center (non-shield sections).
        auto ResolveAnchor(const Section& section, const ModelLink& link) const -> Anchor;
    };
}