export module Blam.MapModel.Vitality.System;

import Common.Log.System;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.World.Type;
import Blam.MapModel.World.State;
import Blam.MapModel.Vitality.Type;
import Blam.MapModel.Vitality.State;
import std;

export namespace Blam::MapModel::Vitality::System
{
    // Builds the vitality layout of every model that has damage sections.
    class VitalityBuilder
    {
    private:
        using Hlmt = Blam::MapModel::Definitions::Type::Hlmt::Hlmt;
        using ModelLink = Blam::MapModel::World::Type::ModelLink::ModelLink;
        using ModelLinkAnchor = Blam::MapModel::World::Type::ModelLink::ModelLinkAnchor;
        using Vitality = Blam::MapModel::Vitality::Type::Vitality::Vitality;
        using Section = Blam::MapModel::Vitality::Type::Vitality::Section;

        using Log = Common::Log::System::Log;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using WorldStore = Blam::MapModel::World::State::WorldStore;
        using VitalityStore = Blam::MapModel::Vitality::State::VitalityStore;

    public:
        VitalityBuilder(Log& log, DefinitionsStore& definitionsStore,
            WorldStore& worldStore, VitalityStore& vitalityStore) : m_Log(log),
            m_DefinitionsStore(definitionsStore), m_WorldStore(worldStore),
            m_VitalityStore(vitalityStore) {}
        ~VitalityBuilder() = default;

        // Builds the layouts from the definitions and the model links, then freezes the store.
        // note: Runs after the definitions and the world are built.
        auto BuildForMap() -> void;

        // Removes every layout.
        auto Cleanup() -> void;

    private:
        Log& m_Log;
        DefinitionsStore& m_DefinitionsStore;
        WorldStore& m_WorldStore;
        VitalityStore& m_VitalityStore;

        // Reads the roles, transfers and anchors of every damage section of the model.
        auto BuildLayout(const Hlmt& hlmt, const ModelLink* link) const -> Vitality;

        // Finds where to aim for a section.
        // note: Cascade: model target of the section, coll region of the section,
        // headshot lock-on model target (headshot sections), object center (non-shield sections).
        auto ResolveAnchor(const Section& section, const ModelLink& link) const -> ModelLinkAnchor;
    };
}