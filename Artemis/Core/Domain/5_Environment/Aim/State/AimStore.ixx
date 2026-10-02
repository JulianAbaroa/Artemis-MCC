export module Environment.Aim.State;

import Environment.Aim.Type;
import std;

export namespace Environment::Aim::State
{
    class AimStore
    {
    private:
        using Aims = Environment::Aim::Type::Aims;

    public:
        AimStore() = default;
        ~AimStore() = default;

        auto Publish(Aims aims) -> void;
        auto Acquire() const -> std::shared_ptr<const Aims>;

        auto Clear() -> void;

    private:
        mutable std::mutex m_Mutex{};
        std::shared_ptr<const Aims> m_Aims{};
    };
}