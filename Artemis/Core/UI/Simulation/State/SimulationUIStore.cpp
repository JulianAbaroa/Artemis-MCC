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

    auto SimulationUIStore::GetCustomTimeScale() const -> float
    {
        return m_CustomTimeScale;
    }

    auto SimulationUIStore::SetCustomTimeScale(float scale) -> void
    {
        m_CustomTimeScale = scale;
    }

    auto SimulationUIStore::GetActionLocalPlayer() const -> std::size_t
    {
        return m_ActionLocalPlayer;
    }

    auto SimulationUIStore::SetActionLocalPlayer(std::size_t player) -> void
    {
        m_ActionLocalPlayer = player;
    }

    auto SimulationUIStore::GetActionBaseline() const -> const std::optional<PlayerAction>&
    {
        return m_ActionBaseline;
    }

    auto SimulationUIStore::SetActionBaseline(std::optional<PlayerAction> baseline) -> void
    {
        m_ActionBaseline = std::move(baseline);
    }
}