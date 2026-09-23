export module Environment.Health.State;

import Environment.Health.Type;
import std;

export namespace Environment::Health::State
{
    class HealthStore
    {
    private:
        using Vitality = Environment::Health::Type::Health;
        using Vitalities = std::unordered_map<std::uint32_t, Vitality>;

    public:
        HealthStore() = default;
        ~HealthStore() = default;

        auto Publish(Vitalities vitalities) -> void;
        auto Acquire() const -> std::shared_ptr<const Vitalities>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const Vitalities>> m_pVitalities;
    };
}