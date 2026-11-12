export module DevTools.Workbench.Simulation.State;

import Common.Engine.Type;
import Engine.Base.MatchSetup.Type;
import DevTools.Workbench.Simulation.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_MaxPlayers;
    using DevTools::Workbench::Simulation::Type::k_RecordNameSize;
    using DevTools::Workbench::Simulation::Type::k_SimulationSectionCount;
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
        using SimulationSection = DevTools::Workbench::Simulation::Type::SimulationSection;
        using Roster = Engine::Base::MatchSetup::Type::Roster;

    public:
        // Prefix of the keys of this store in the preferences file.
        static constexpr std::string_view k_SimulationPreferencePrefix{ "Simulation_" };

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

        // Action mode of a bot, by its player table index and the handle of the player in it (A9 M5b). A bot that replaces another at
        // the same index has another handle (the salt changes), so it starts with the engine action.
        auto GetBotAction(std::size_t playerIndex, std::uint32_t handle) const -> LocalPlayerAction;
        auto SetBotAction(std::size_t playerIndex, std::uint32_t handle, LocalPlayerAction action) -> void;

        // Team of the next bots in team games, or nullopt for the team of the local player.
        auto GetNewBotTeam() const -> std::optional<std::int8_t>;
        auto SetNewBotTeam(std::optional<std::int8_t> team) -> void;

        // Counts typed in the roster editor of the bots section, sent with its button.
        auto GetRosterEditor() const -> Roster;
        auto SetRosterEditor(const Roster& roster) -> void;

        // Player table index whose detail the session list shows.
        auto GetSessionPlayer() const -> std::size_t;
        auto SetSessionPlayer(std::size_t playerIndex) -> void;

        // Whether a section of the tab is unfolded. All but Development start unfolded.
        auto IsSectionOpen(SimulationSection section) const -> bool;
        auto SetSectionOpen(SimulationSection section, bool isOpen) -> void;

        // Whether the player actions window is open. Not saved.
        auto IsPlayerActionsOpen() const -> bool;
        auto SetPlayerActionsOpen(bool isOpen) -> void;

        // Writes the open sections as "<prefix><key>=<0|1>" lines.
        auto Save(std::ostream& stream) const -> void;

        // Applies one saved key. Ignores unknown keys and values that cannot be read.
        auto Load(std::string_view key, std::string_view value) -> void;

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
        std::array<std::uint32_t, k_MaxPlayers> m_BotActionHandles{};
        std::size_t m_SessionPlayer{ 0 };
        std::array<bool, k_SimulationSectionCount> m_OpenSections{ true, true, true, true, false };
        bool m_IsPlayerActionsOpen{ false };
        std::optional<std::int8_t> m_NewBotTeam{};
        Roster m_RosterEditor{};
        std::array<char, k_RecordNameSize> m_RecordNameField{};
        std::vector<std::string> m_MatchConfigNames{};
        std::size_t m_SelectedMatchConfig{ 0 };
    };
}