export module DevTools.Workbench.Simulation.State;

import Common.Engine.Type;
import Engine.Control.Simulation.Type;
import DevTools.Workbench.Simulation.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_MaxPlayers;
    using DevTools::Workbench::Simulation::Type::k_RecordNameSize;
}

export namespace DevTools::Workbench::Simulation::State
{
    // Choices of the simulation tab that are not stored elsewhere.
    // note: UI thread only.
    class SimulationUIStore
    {
    private:
        using LocalPlayerInput = DevTools::Workbench::Simulation::Type::LocalPlayerInput;
        using LocalPlayerAction = DevTools::Workbench::Simulation::Type::LocalPlayerAction;

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

        // Action mode of a bot, by its player table index (A9 M5b).
        auto GetBotAction(std::size_t playerIndex) const -> LocalPlayerAction;
        auto SetBotAction(std::size_t playerIndex, LocalPlayerAction action) -> void;

        // Team of the next bots in team games, or nullopt for the team of the local player.
        auto GetNewBotTeam() const -> std::optional<std::int8_t>;
        auto SetNewBotTeam(std::optional<std::int8_t> team) -> void;

        // Player whose game options entry the session section shows.
        auto GetOptionsPlayer() const -> std::size_t;
        auto SetOptionsPlayer(std::size_t playerIndex) -> void;

        // Name typed for the next record of the match config section (D123).
        auto GetRecordNameField() -> std::array<char, k_RecordNameSize>&;

        // Recorded match configurations the last refresh listed, and the one picked in the list.
        auto GetMatchConfigNames() const -> const std::vector<std::string>&;
        auto SetMatchConfigNames(std::vector<std::string> names) -> void;
        auto GetSelectedMatchConfig() const -> std::size_t;
        auto SetSelectedMatchConfig(std::size_t index) -> void;

    private:
        std::array<LocalPlayerInput, k_MaxLocalUsers> m_LocalPlayerInputs{};
        float m_CustomTimeScale{ 1.0f };
        std::size_t m_ActionLocalPlayer{ 0 };
        std::array<LocalPlayerAction, k_MaxLocalUsers> m_LocalPlayerActions{};
        std::array<LocalPlayerAction, k_MaxPlayers> m_BotActions{};
        std::size_t m_OptionsPlayer{ 0 };
        std::optional<std::int8_t> m_NewBotTeam{};
        std::array<char, k_RecordNameSize> m_RecordNameField{};
        std::vector<std::string> m_MatchConfigNames{};
        std::size_t m_SelectedMatchConfig{ 0 };
    };
}