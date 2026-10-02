export module Resolved.World.State;

import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import Resolved.World.Type;
import std;

export namespace Resolved::World::State
{
    class WorldStore
    {
    private:
        using ResolvedColl = Resolved::Definitions::Type::Coll::Coll;
        using ResolvedRegionStates = Resolved::World::Type::RegionStates::RegionStates;
        using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;

    public:
        WorldStore() = default;
        ~WorldStore() = default;

        // Coll: 
        auto GetResolvedCollForObject(const std::string& objectTagName, const DefinitionsStore& definitionsStore) const -> const ResolvedColl*;
        auto GetResolvedRegionStates(const std::string& objectTagName) const -> const ResolvedRegionStates*;
        auto AddResolvedRegionStates(const std::string& hlmtTagName, ResolvedRegionStates states) -> void;

        auto GetNodeCount(const std::string& tagName, const DefinitionsStore& definitionsStore) const -> std::size_t;

        auto ResolveObjectCollName(const std::string& objectTagName) const -> std::string;
        auto LinkObjectColl(const std::string& objectTagName, const std::string& collTagName) -> void;

        auto ResolveObjectHlmtName(const std::string& objectTagName) const -> std::string;
        auto LinkObjectHlmt(const std::string& objectTagName, const std::string& hlmtTagName) -> void;

        auto IsFrozen() const -> bool
        {
            return m_Frozen.load(std::memory_order_acquire);
        }

        auto Freeze() -> void;

        auto Cleanup() -> void;

    private:
        auto CompileObjectIndices() -> void;


        std::vector<ResolvedRegionStates> m_ResolvedRegionStates{};
        std::unordered_map<std::string, std::int32_t> m_RegionStatesIndexByName{};

        std::unordered_map<std::string, std::string> m_ObjectColls{};
        std::unordered_map<std::string, std::string> m_ObjectHlmts{};

        std::unordered_map<std::string, std::int32_t> m_ObjectHlmtIndex{};

        std::atomic<bool> m_Frozen{ false };
    };
}