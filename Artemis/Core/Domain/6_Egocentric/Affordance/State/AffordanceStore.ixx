export module Egocentric.Affordance.State;

import Egocentric.Affordance.Type;
import std;

export namespace Egocentric::Affordance::State
{
    class AffordanceStore
    {
    private:
        using Affordance = Egocentric::Affordance::Type::Affordance;
        using Affordances = std::vector<Affordance>;

    public:
        AffordanceStore() = default;
        ~AffordanceStore() = default;

        auto Publish(Affordances interactables) -> void;
        auto Acquire() const -> std::shared_ptr<const Affordances>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const Affordances>> m_pAffordances;
    };
}