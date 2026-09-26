export module Resolved.Stats.State;

import Resolved.Stats.Type;
import std;

export namespace Resolved::Stats::State
{
    class StatsStore
    {
    private:
        using ResolvedVehi = Resolved::Stats::Type::Vehi::Vehi;
        using ResolvedWeap = Resolved::Stats::Type::Weap::Weap;
        using ResolvedProj = Resolved::Stats::Type::Proj::Proj;

    public:
        StatsStore() = default;
        ~StatsStore() = default;

        // Vehi.
        auto HasResolvedVehi(const std::string& tagName) const -> bool;
        auto GetResolvedVehi(const std::string& tagName) const -> const ResolvedVehi*;
        auto AddResolvedVehi(const std::string& tagName, ResolvedVehi data) -> void;

        // Weap.
        auto HasResolvedWeap(const std::string& tagName) const -> bool;
        auto GetResolvedWeap(const std::string& tagName) const -> const ResolvedWeap*;
        auto AddResolvedWeap(const std::string& tagName, ResolvedWeap data) -> void;

        // Proj.
        auto HasResolvedProj(const std::string& tagName) const -> bool;
        auto GetResolvedProj(const std::string& tagName) const -> const ResolvedProj*;
        auto AddResolvedProj(const std::string& tagName, ResolvedProj data) -> void;

        auto IsFrozen() const -> bool
        {
            return m_Frozen.load(std::memory_order_acquire);
        }

        auto Freeze() -> void
        {
            m_Frozen.store(true, std::memory_order_release);
        }

        auto Cleanup() -> void;

    private:
        std::unordered_map<std::string, ResolvedVehi> m_ResolvedVehis{};
        std::unordered_map<std::string, ResolvedWeap> m_ResolvedWeaps{};
        std::unordered_map<std::string, ResolvedProj> m_ResolvedProjs{};

        std::atomic<bool> m_Frozen{ false };
    };
}