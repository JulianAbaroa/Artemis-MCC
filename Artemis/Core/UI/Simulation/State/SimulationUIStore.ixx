export module UI.Simulation.State;

import Platform.Host.Type;
import UI.Simulation.Type;
import std;

namespace
{
    using Platform::Host::Type::k_MaxLocalUsers;
}

export namespace UI::Simulation::State
{
    // Choices of the simulation tab that are not stored elsewhere.
    // note: UI thread only.
    class SimulationUIStore
    {
    private:
        using LocalPlayerInput = UI::Simulation::Type::LocalPlayerInput;

    public:
        SimulationUIStore() = default;
        ~SimulationUIStore() = default;

        SimulationUIStore(const SimulationUIStore&) = delete;
        auto operator=(const SimulationUIStore&) -> SimulationUIStore& = delete;

        auto GetLocalPlayerInput(std::size_t player) const -> LocalPlayerInput;
        auto SetLocalPlayerInput(std::size_t player, LocalPlayerInput input) -> void;

        // Value of the custom speed field, applied with its button.
        auto GetCustomTimeScale() const -> float;
        auto SetCustomTimeScale(float scale) -> void;

    private:
        std::array<LocalPlayerInput, k_MaxLocalUsers> m_LocalPlayerInputs{};
        float m_CustomTimeScale{ 1.0f };
    };
}