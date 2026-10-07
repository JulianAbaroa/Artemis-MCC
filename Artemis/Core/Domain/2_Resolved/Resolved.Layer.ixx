export module Resolved.Layer;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Resolved.Definitions.State;
import Resolved.Definitions.System;
import Resolved.World.State;
import Resolved.World.System;
import Resolved.Vitality.State;
import Resolved.Vitality.System;

export namespace Resolved
{
    // Resolves the static data of the map: the definitions of its tags, the world built from them
    // (links, region states and raycast grid), and the vitality layouts of its models.
    class Layer
    {
    private:
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using WorldStore = Resolved::World::State::WorldStore;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

        using DefinitionsBuilder = Resolved::Definitions::System::DefinitionsBuilder;
        using RegionStatesBuilder = Resolved::World::System::RegionStatesBuilder;
        using ModelLinkBuilder = Resolved::World::System::ModelLinkBuilder;
        using Raycaster = Resolved::World::System::Raycaster;
        using WorldBuilder = Resolved::World::System::WorldBuilder;
        using VitalityBuilder = Resolved::Vitality::System::VitalityBuilder;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map) :
            m_DefinitionsBuilder(service.m_LogsService, map.m_TagCatalog, map.m_GeometryLoaderService, m_DefinitionsStore, map.m_TagResolverService),
            m_WorldBuilder(service.m_LogsService, m_DefinitionsStore, m_WorldStore, m_Raycaster, m_RegionStatesBuilder, m_ModelLinkBuilder),
            m_VitalityBuilder(service.m_LogsService, m_DefinitionsStore, m_WorldStore, m_VitalityStore)
        {
            platform.m_LifecycleService.OnCleanup([this] {
                m_WorldBuilder.Cleanup();
                m_DefinitionsBuilder.Cleanup();
                m_VitalityBuilder.Cleanup();
            });
        }
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        DefinitionsStore m_DefinitionsStore;
        WorldStore m_WorldStore;
        VitalityStore m_VitalityStore;

        // --- System ---
        DefinitionsBuilder m_DefinitionsBuilder;
        RegionStatesBuilder m_RegionStatesBuilder;
        ModelLinkBuilder m_ModelLinkBuilder;
        Raycaster m_Raycaster{};
        WorldBuilder m_WorldBuilder;
        VitalityBuilder m_VitalityBuilder;
    };
}