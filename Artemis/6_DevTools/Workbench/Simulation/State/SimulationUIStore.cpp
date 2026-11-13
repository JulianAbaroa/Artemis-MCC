module DevTools.Workbench.Simulation.State;

namespace
{
    using DevTools::Workbench::Simulation::Type::k_RecordNameSize;
    using DevTools::Workbench::Simulation::Type::k_SimulationSectionKeys;
}

namespace DevTools::Workbench::Simulation::State
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

    auto SimulationUIStore::GetBotAction(std::size_t playerIndex, std::uint32_t handle) const -> LocalPlayerAction
    {
        return m_BotActionHandles[playerIndex] == handle ? m_BotActions[playerIndex] : LocalPlayerAction::Engine;
    }

    auto SimulationUIStore::SetBotAction(std::size_t playerIndex, std::uint32_t handle, LocalPlayerAction action) -> void
    {
        m_BotActionHandles[playerIndex] = handle;
        m_BotActions[playerIndex] = action;
    }

    auto SimulationUIStore::GetRosterEditor() const -> Roster
    {
        return m_RosterEditor;
    }

    auto SimulationUIStore::SetRosterEditor(const Roster& roster) -> void
    {
        m_RosterEditor = roster;
    }

    auto SimulationUIStore::GetSessionPlayer() const -> std::size_t
    {
        return m_SessionPlayer;
    }

    auto SimulationUIStore::SetSessionPlayer(std::size_t playerIndex) -> void
    {
        m_SessionPlayer = playerIndex;
    }

    auto SimulationUIStore::IsSectionOpen(SimulationSection section) const -> bool
    {
        return m_OpenSections[static_cast<std::size_t>(section)];
    }

    auto SimulationUIStore::SetSectionOpen(SimulationSection section, bool isOpen) -> void
    {
        m_OpenSections[static_cast<std::size_t>(section)] = isOpen;
    }

    auto SimulationUIStore::IsPlayerActionsOpen() const -> bool
    {
        return m_IsPlayerActionsOpen;
    }

    auto SimulationUIStore::SetPlayerActionsOpen(bool isOpen) -> void
    {
        m_IsPlayerActionsOpen = isOpen;
    }

    auto SimulationUIStore::Save(std::ostream& stream) const -> void
    {
        for (std::size_t section{ 0 }; section < m_OpenSections.size(); ++section)
        {
            stream << k_SimulationPreferencePrefix << k_SimulationSectionKeys[section] << '=' << (m_OpenSections[section] ? 1 : 0) << '\n';
        }
    }

    auto SimulationUIStore::Load(std::string_view key, std::string_view value) -> void
    {
        if (value != "0" && value != "1") return;

        for (std::size_t section{ 0 }; section < m_OpenSections.size(); ++section)
        {
            if (key == k_SimulationSectionKeys[section]) m_OpenSections[section] = value == "1";
        }
    }

    auto SimulationUIStore::GetSelectedBot() const -> std::size_t
    {
        return m_SelectedBot;
    }

    auto SimulationUIStore::SetSelectedBot(std::size_t number) -> void
    {
        m_SelectedBot = number;
    }

    auto SimulationUIStore::GetRecordNameField() -> std::array<char, k_RecordNameSize>&
    {
        return m_RecordNameField;
    }

    auto SimulationUIStore::GetRenameField() -> std::array<char, k_RecordNameSize>&
    {
        return m_RenameField;
    }

    auto SimulationUIStore::GetConfigEditError() const -> const std::string&
    {
        return m_ConfigEditError;
    }

    auto SimulationUIStore::SetConfigEditError(std::string error) -> void
    {
        m_ConfigEditError = std::move(error);
    }

    auto SimulationUIStore::IsConfigComboOpen() const -> bool
    {
        return m_IsConfigComboOpen;
    }

    auto SimulationUIStore::SetConfigComboOpen(bool isOpen) -> void
    {
        m_IsConfigComboOpen = isOpen;
    }

    auto SimulationUIStore::IsConfigListLoaded() const -> bool
    {
        return m_IsConfigListLoaded;
    }

    auto SimulationUIStore::SetConfigListLoaded(bool isLoaded) -> void
    {
        m_IsConfigListLoaded = isLoaded;
    }

    auto SimulationUIStore::GetMatchConfigNames() const -> const std::vector<std::string>&
    {
        return m_MatchConfigNames;
    }

    auto SimulationUIStore::SetMatchConfigNames(std::vector<std::string> names) -> void
    {
        m_MatchConfigNames = std::move(names);
    }

    auto SimulationUIStore::GetSelectedMatchConfig() const -> std::size_t
    {
        return m_SelectedMatchConfig;
    }

    auto SimulationUIStore::SetSelectedMatchConfig(std::size_t index) -> void
    {
        m_SelectedMatchConfig = index;
    }

    auto SimulationUIStore::GetMapVariantEntries() const -> const std::vector<MapVariantEntry>&
    {
        return m_MapVariantEntries;
    }

    auto SimulationUIStore::SetMapVariantEntries(std::vector<MapVariantEntry> entries) -> void
    {
        m_MapVariantEntries = std::move(entries);
    }

    auto SimulationUIStore::GetSelectedMapVariant() const -> std::size_t
    {
        return m_SelectedMapVariant;
    }

    auto SimulationUIStore::SetSelectedMapVariant(std::size_t index) -> void
    {
        m_SelectedMapVariant = index;
    }

    auto SimulationUIStore::IsMapVariantComboOpen() const -> bool
    {
        return m_IsMapVariantComboOpen;
    }

    auto SimulationUIStore::SetMapVariantComboOpen(bool isOpen) -> void
    {
        m_IsMapVariantComboOpen = isOpen;
    }

    auto SimulationUIStore::GetCreateTemplate() const -> const std::string&
    {
        return m_CreateTemplate;
    }

    auto SimulationUIStore::SetCreateTemplate(std::string name) -> void
    {
        m_CreateTemplate = std::move(name);
    }

    auto SimulationUIStore::GetCreateNameField() -> std::array<char, k_RecordNameSize>&
    {
        return m_CreateNameField;
    }

    auto SimulationUIStore::GetCreatePrefilledFor() const -> const std::filesystem::path&
    {
        return m_CreatePrefilledFor;
    }

    auto SimulationUIStore::SetCreatePrefilledFor(std::filesystem::path file) -> void
    {
        m_CreatePrefilledFor = std::move(file);
    }

    auto SimulationUIStore::GetMapVariantCreate() -> MapVariantCreate&
    {
        return m_MapVariantCreate;
    }
}