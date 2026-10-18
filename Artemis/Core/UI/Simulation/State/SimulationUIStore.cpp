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

    auto SimulationUIStore::GetLocalPlayerAction(std::size_t player) const -> LocalPlayerAction
    {
        return m_LocalPlayerActions[player];
    }

    auto SimulationUIStore::SetLocalPlayerAction(std::size_t player, LocalPlayerAction action) -> void
    {
        m_LocalPlayerActions[player] = action;
    }

    auto SimulationUIStore::GetBotAction(std::size_t playerIndex) const -> LocalPlayerAction
    {
        return m_BotActions[playerIndex];
    }

    auto SimulationUIStore::SetBotAction(std::size_t playerIndex, LocalPlayerAction action) -> void
    {
        m_BotActions[playerIndex] = action;
    }

    auto SimulationUIStore::GetOptionsPlayer() const -> std::size_t
    {
        return m_OptionsPlayer;
    }

    auto SimulationUIStore::SetOptionsPlayer(std::size_t playerIndex) -> void
    {
        m_OptionsPlayer = playerIndex;
    }

    auto SimulationUIStore::GetNewBotTeam() const -> std::optional<std::int8_t>
    {
        return m_NewBotTeam;
    }

    auto SimulationUIStore::SetNewBotTeam(std::optional<std::int8_t> team) -> void
    {
        m_NewBotTeam = team;
    }
}