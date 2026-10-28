export module Resolved.World.State;

import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Resolved.World.Type;
import std;

export namespace Resolved::World::State
{
    // Links of the world by tag name: object to model, model to collision, and the region states and model links of each model.
    // note: They are added while the world is built, then the store is frozen and only read.
    // Reads assert that the store is frozen, and Link and Add assert that it is not.
    class WorldStore
    {
    private:
        using Coll = Resolved::Definitions::Type::Coll::Coll;
        using RegionStates = Resolved::World::Type::RegionStates::RegionStates;
        using ModelLink = Resolved::World::Type::ModelLink::ModelLink;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;

    public:
        WorldStore() = default;
        ~WorldStore() = default;

        // return: Null if the object has no collision model.
        auto GetCollForObject(const std::string& objectTagName, const DefinitionsStore& definitionsStore) const -> const Coll*;

        // return: Null if the object has no region states.
        auto GetRegionStates(const std::string& objectTagName) const -> const RegionStates*;
        auto AddRegionStates(const std::string& hlmtTagName, RegionStates states) -> void;

        // return: Null if the model has no link.
        auto GetModelLink(const std::string& hlmtTagName) const -> const ModelLink*;
        auto AddModelLink(const std::string& hlmtTagName, ModelLink link) -> void;

        // return: Empty if the object has no collision model.
        auto ResolveObjectCollName(const std::string& objectTagName) const -> std::string;
        auto LinkObjectColl(const std::string& objectTagName, const std::string& collTagName) -> void;

        // return: Empty if the object has no model.
        auto ResolveObjectHlmtName(const std::string& objectTagName) const -> std::string;
        auto LinkObjectHlmt(const std::string& objectTagName, const std::string& hlmtTagName) -> void;

        auto IsFrozen() const -> bool
        {
            return m_IsFrozen.load(std::memory_order_acquire);
        }

        // Ends the build. From now on the store can only be read.
        auto Freeze() -> void
        {
            this->CompileObjectIndices();
            m_IsFrozen.store(true, std::memory_order_release);
        }

        // Removes every link and unfreezes the store.
        auto Cleanup() -> void;

    private:
        std::vector<RegionStates> m_RegionStates{};
        std::unordered_map<std::string, std::int32_t> m_RegionStatesIndexByName{};

        std::unordered_map<std::string, ModelLink> m_ModelLinks{};

        std::unordered_map<std::string, std::string> m_ObjectColls{};
        std::unordered_map<std::string, std::string> m_ObjectHlmts{};

        // Index into m_RegionStates of each object.
        std::unordered_map<std::string, std::int32_t> m_ObjectHlmtIndex{};

        std::atomic<bool> m_IsFrozen{ false };

        // Joins each object with the region states of its model, so reads need a single lookup.
        auto CompileObjectIndices() -> void;
    };
}