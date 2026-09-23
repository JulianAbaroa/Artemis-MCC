export module Environment.Fixtures.State;

import Environment.Fixtures.Type;
import std;

export namespace Environment::Fixtures::State
{
    class FixturesStore
    {
    private:
        using Fixtures = Environment::Fixtures::Type::Fixtures;

    public:
        FixturesStore() = default;
        ~FixturesStore() = default;

        auto Publish(Fixtures data) -> void;
        auto Acquire() const -> std::shared_ptr<const Fixtures>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const Fixtures>> m_pFixtures;
    };
}