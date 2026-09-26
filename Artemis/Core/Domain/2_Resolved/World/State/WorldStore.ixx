export module Resolved.World.State;

import Resolved.World.Type;
import std;

export namespace Resolved::World::State
{
    class WorldStore
    {
    private:
        using ResolvedColl = Resolved::World::Type::Coll::Coll;
        using ResolvedMode = Resolved::World::Type::Mode::Mode;
        using ResolvedSbsp = Resolved::World::Type::Sbsp::Sbsp;
        using ResolvedRegionStates = Resolved::World::Type::RegionStates::RegionStates;

    public:
        WorldStore() = default;
        ~WorldStore() = default;

        // Coll.
        auto HasResolvedColl(const std::string& tagName) const -> bool;
        auto GetResolvedColl(const std::string& tagName) const -> const ResolvedColl*;
        auto AddResolvedColl(const std::string& tagName, ResolvedColl coll) -> void;
        auto PeekResolvedColl(const std::string& tagName) const -> const ResolvedColl*;
        auto GetResolvedCollForObject(const std::string& objectTagName) const -> const ResolvedColl*;

        auto GetResolvedRegionStates(const std::string& objectTagName) const -> const ResolvedRegionStates*;
        auto AddResolvedRegionStates(const std::string& hlmtTagName, ResolvedRegionStates states) -> void;

        // Mode.
        auto HasResolvedMode(const std::string& tagName) const -> bool;
        auto GetResolvedMode(const std::string& tagName) const -> const ResolvedMode*;
        auto AddResolvedMode(const std::string& tagName, ResolvedMode mode) -> void;

        // Sbsp.
        auto HasResolvedSbsp() const -> bool;
        auto GetResolvedSbsp(std::int32_t index) const -> const ResolvedSbsp*;
        auto AddResolvedSbsp(ResolvedSbsp sbsp) -> void;
        auto GetAllResolvedSbsps() const -> const std::vector<ResolvedSbsp>&;
        auto GetResolvedSbspCount() const -> std::int32_t;

        auto GetNodeCount(const std::string& tagName) const -> std::size_t;

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

        std::vector<ResolvedColl> m_ResolvedColls{};
        std::unordered_map<std::string, std::int32_t> m_CollIndexByName{};

        std::vector<ResolvedMode> m_ResolvedModes{};
        std::unordered_map<std::string, std::int32_t> m_ModeIndexByName{};

        std::vector<ResolvedSbsp> m_ResolvedSbsps{};

        std::vector<ResolvedRegionStates> m_ResolvedRegionStates{};
        std::unordered_map<std::string, std::int32_t> m_RegionStatesIndexByName{};

        std::unordered_map<std::string, std::string> m_ObjectColls{};
        std::unordered_map<std::string, std::string> m_ObjectHlmts{};

        std::unordered_map<std::string, std::int32_t> m_ObjectCollIndex{};
        std::unordered_map<std::string, std::int32_t> m_ObjectHlmtIndex{};

        std::atomic<bool> m_Frozen{ false };
    };
}