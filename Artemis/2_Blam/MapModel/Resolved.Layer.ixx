export module Resolved.Layer;

import Common.Log.System;
import Map.Layer;
import Resolved.Tag.System;
import Resolved.Definitions.State;
import Resolved.Definitions.System;
import Resolved.World.State;
import Resolved.World.System;
import Resolved.Vitality.State;
import Resolved.Vitality.System;

export namespace Resolved
{
    // Resolves the static data of the map: the definitions of its tags, the world built from them
    // (links, region states and the raycast hierarchy), and the vitality layouts of its models. Also resolves tag handles.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using WorldStore = Resolved::World::State::WorldStore;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;

        using TagResolverService = Resolved::Tag::System::TagResolverService;
        using DefinitionsBuilder = Resolved::Definitions::System::DefinitionsBuilder;
        using RegionStatesBuilder = Resolved::World::System::RegionStatesBuilder;
        using ModelLinkBuilder = Resolved::World::System::ModelLinkBuilder;
        using Raycaster = Resolved::World::System::Raycaster;
        using WorldBuilder = Resolved::World::System::WorldBuilder;
        using VitalityBuilder = Resolved::Vitality::System::VitalityBuilder;

    public:
        Layer(Log& log, Map::Layer& map) :
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