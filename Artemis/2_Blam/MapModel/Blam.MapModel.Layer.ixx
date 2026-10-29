export module Blam.MapModel.Layer;

import Common.Log.System;
import Blam.Map.Layer;
import Blam.MapModel.Tag.System;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.Definitions.System;
import Blam.MapModel.World.State;
import Blam.MapModel.World.System;
import Blam.MapModel.Vitality.State;
import Blam.MapModel.Vitality.System;

export namespace Blam::MapModel
{
    // Resolves the static data of the map: the definitions of its tags, the world built from them
    // (links, region states and the raycast hierarchy), and the vitality layouts of its models. Also resolves tag handles.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using WorldStore = Blam::MapModel::World::State::WorldStore;
        using VitalityStore = Blam::MapModel::Vitality::State::VitalityStore;

        using TagResolverService = Blam::MapModel::Tag::System::TagResolverService;
        using DefinitionsBuilder = Blam::MapModel::Definitions::System::DefinitionsBuilder;
        using RegionStatesBuilder = Blam::MapModel::World::System::RegionStatesBuilder;
        using ModelLinkBuilder = Blam::MapModel::World::System::ModelLinkBuilder;
        using Raycaster = Blam::MapModel::World::System::Raycaster;
        using WorldBuilder = Blam::MapModel::World::System::WorldBuilder;
        using VitalityBuilder = Blam::MapModel::Vitality::System::VitalityBuilder;

    public:
        Layer(Log& log, Blam::Map::Layer& map) :
            m_TagResolverService(log, map.m_FileStore, map.m_TagIndexStore),
            m_DefinitionsBuilder(log, map.m_TagCatalog, map.m_GeometryLoaderService, m_DefinitionsStore, map.m_TagReferenceService),
            m_WorldBuilder(log, m_DefinitionsStore, m_WorldStore, m_Raycaster, m_RegionStatesBuilder, m_ModelLinkBuilder),
            m_VitalityBuilder(log, m_DefinitionsStore, m_WorldStore, m_VitalityStore) {}
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        DefinitionsStore m_DefinitionsStore{};
        WorldStore m_WorldStore{};
        VitalityStore m_VitalityStore{};

        // --- System ---
        TagResolverService m_TagResolverService;
        DefinitionsBuilder m_DefinitionsBuilder;
        RegionStatesBuilder m_RegionStatesBuilder{};
        ModelLinkBuilder m_ModelLinkBuilder{};
        Raycaster m_Raycaster{};
        WorldBuilder m_WorldBuilder;
        VitalityBuilder m_VitalityBuilder;

        // Removes the world, the definitions, the vitality layouts and the indexed tag names.
        auto Cleanup() -> void
        {
            m_WorldBuilder.Cleanup();
            m_DefinitionsBuilder.Cleanup();
            m_VitalityBuilder.Cleanup();
            m_TagResolverService.Cleanup();
        }
    };
}