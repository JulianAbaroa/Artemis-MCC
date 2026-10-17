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
        using LocalPlayerAction = UI::Simulation::Type::LocalPlayerAction;

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

        // Local player whose action the player actions section shows.
        auto GetActionLocalPlayer() const -> std::size_t;
        auto SetActionLocalPlayer(std::size_t player) -> void;

        auto GetLocalPlayerAction(std::size_t player) const -> LocalPlayerAction;
        auto SetLocalPlayerAction(std::size_t player, LocalPlayerAction action) -> void;

    private:
        std::array<LocalPlayerInput, k_MaxLocalUsers> m_LocalPlayerInputs{};
        float m_CustomTimeScale{ 1.0f };
        std::size_t m_ActionLocalPlayer{ 0 };
        std::array<LocalPlayerAction, k_MaxLocalUsers> m_LocalPlayerActions{};
    };
}