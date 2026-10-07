export module Resolved.World.System;

export import :ModelLink;
export import :Raycast;
export import :RegionStates;

import Service.Logs.System;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Resolved.World.State;
import std;

export namespace Resolved::World::System
{
    // Builds the world from the definitions: which model and collision each object has, how its
    // collision permutations change, how its collision and render models relate, and the hierarchy that rays are cast against.
    class WorldBuilder
    {
    private:
        using Hlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using Coll = Resolved::Definitions::Type::Coll::Coll;
        using Mode = Resolved::Definitions::Type::Mode::Mode;

        using LogsService = Service::Logs::System::LogsService;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
        using WorldStore = Resolved::World::State::WorldStore;
        using Raycaster = Resolved::World::System::Raycaster;
        using RegionStatesBuilder = Resolved::World::System::RegionStatesBuilder;
        using ModelLinkBuilder = Resolved::World::System::ModelLinkBuilder;

    public:
        WorldBuilder(LogsService& logsService, DefinitionsStore& definitionsStore,
            WorldStore& worldStore, Raycaster& raycaster,
            RegionStatesBuilder& regionStatesBuilder,
            ModelLinkBuilder& modelLinkBuilder) : m_LogsService(logsService),
            m_DefinitionsStore(definitionsStore), m_WorldStore(worldStore),
            m_Raycaster(raycaster), m_RegionStatesBuilder(regionStatesBuilder),
            m_ModelLinkBuilder(modelLinkBuilder) {}
        ~WorldBuilder() = default;

        // Builds the world of the loaded map and freezes the store.
        // note: Runs after the definitions are built.
        auto BuildForMap() -> void;

        // Removes the world.
        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        DefinitionsStore& m_DefinitionsStore;
        WorldStore& m_WorldStore;
        Raycaster& m_Raycaster;
        RegionStatesBuilder& m_RegionStatesBuilder;
        ModelLinkBuilder& m_ModelLinkBuilder;

        // Links every object with its model and collision model, and builds the region states of each model once.
        auto LinkObjects() -> void;

        template <typename TStore>
        auto LinkObjectFamily(const TStore& definitions,
            std::unordered_set<std::string>& builtRegionStates) -> void;

        auto BuildRegionStates(const std::string& hlmtName, const Hlmt& hlmt) -> void;

        // Builds the model link of every model that has damage sections.
        auto BuildModelLinks() -> void;
    };
}