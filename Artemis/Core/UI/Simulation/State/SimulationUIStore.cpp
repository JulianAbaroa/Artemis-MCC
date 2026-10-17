module UI.Simulation.State;

namespace UI::Simulation::State
{
    auto SimulationUIStore::GetLocalPlayerInput(std::size_t player) const -> LocalPlayerInput
    {
        return m_LocalPlayerInputs[player];
    }

    auto SimulationUIStore::SetLocalPlayerInput(std::size_t player, LocalPlayerInput input) -> void
    {
        m_LocalPlayerInputs[player] = input;
    }
}