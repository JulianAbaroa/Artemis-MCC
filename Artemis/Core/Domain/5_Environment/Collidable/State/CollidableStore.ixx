export module Environment.Collidable.State;

import Environment.Collidable.Type;
import std;

export namespace Environment::Collidable::State
{
    class CollidableStore
    {
    private:
        using Collidable = Environment::Collidable::Type::Collidable;
        using Collidables = std::vector<Collidable>;

    public:
        CollidableStore() = default;
        ~CollidableStore() = default;

        auto Publish(Collidables instances) -> void;
        auto Acquire() const -> std::shared_ptr<const Collidables>;

        auto Cleanup() -> void;

    private:
        std::atomic<std::shared_ptr<const Collidables>> m_pCollidables;
    };
}