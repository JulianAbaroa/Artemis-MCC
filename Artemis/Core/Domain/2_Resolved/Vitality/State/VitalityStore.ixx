export module Resolved.Vitality.State;

import Resolved.Vitality.Type;
import std;

export namespace Resolved::Vitality::State
{
    class VitalityStore
    {
    private:
        using ResolvedVitality = Resolved::Vitality::Type::Vitality;

    public:
        VitalityStore() = default;
        ~VitalityStore() = default;

        auto HasResolvedVitality(const std::string& tagName) const -> bool;
        auto GetResolvedVitality(const std::string& tagName) const -> const ResolvedVitality*;
        auto AddResolvedVitality(const std::string& tagName, ResolvedVitality layout) -> void;

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
        std::unordered_map<std::string, ResolvedVitality> m_ResolvedVitalities;
        std::atomic<bool> m_Frozen{ false };
    };
}