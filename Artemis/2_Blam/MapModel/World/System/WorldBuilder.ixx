export module Blam.MapModel.World.System;

export import :ModelLink;
export import :Raycast;
export import :RegionStates;

import Common.Log.System;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.World.State;
import std;

export namespace Blam::MapModel::World::System
{
    // Builds the world from the definitions: which model and collision each object has, how its
    // collision permutations change, how its collision and render models relate, and the hierarchy that rays are cast against.
    class WorldBuilder
    {
    private:
        using Hlmt = Blam::MapModel::Definitions::Type::Hlmt::Hlmt;
        using Coll = Blam::MapModel::Definitions::Type::Coll::Coll;
        using Mode = Blam::MapModel::Definitions::Type::Mode::Mode;

        using Log = Common::Log::System::Log;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using WorldStore = Blam::MapModel::World::State::WorldStore;
        using Raycaster = Blam::MapModel::World::System::Raycaster;
        using RegionStatesBuilder = Blam::MapModel::World::System::RegionStatesBuilder;
        using ModelLinkBuilder = Blam::MapModel::World::System::ModelLinkBuilder;

    public:
        WorldBuilder(Log& log, DefinitionsStore& definitionsStore,
            WorldStore& worldStore, Raycaster& raycaster,
            RegionStatesBuilder& regionStatesBuilder,
            ModelLinkBuilder& modelLinkBuilder) : m_Log(log),
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
        Log& m_Log;
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