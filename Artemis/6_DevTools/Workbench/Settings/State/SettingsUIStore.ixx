export module DevTools.Workbench.Settings.State;

import DevTools.Workbench.Settings.Type;
import std;

namespace
{
    using DevTools::Workbench::Settings::Type::k_SettingsSectionCount;
}

export namespace DevTools::Workbench::Settings::State
{
    // Choices of the settings tab that are not stored elsewhere: which sections are unfolded.
    // note: UI thread only.
    class SettingsUIStore
    {
    private:
        using SettingsSection = DevTools::Workbench::Settings::Type::SettingsSection;

    public:
        // Prefix of the keys of this store in the preferences file. It must not start with "Settings_", which the preferences service
        // keeps for the settings of SettingsStore.
        static constexpr std::string_view k_SettingsPreferencePrefix{ "SettingsTab_" };

        SettingsUIStore() = default;
        ~SettingsUIStore() = default;

        SettingsUIStore(const SettingsUIStore&) = delete;
        auto operator=(const SettingsUIStore&) -> SettingsUIStore& = delete;

        // Whether a section of the tab is unfolded. Data and Viewer start unfolded.
        auto IsSectionOpen(SettingsSection section) const -> bool;
        auto SetSectionOpen(SettingsSection section, bool isOpen) -> void;

        // Writes the open sections as "<prefix><key>=<0|1>" lines.
        auto Save(std::ostream& stream) const -> void;

        // Applies one saved key. Ignores unknown keys and values that cannot be read.
        auto Load(std::string_view key, std::string_view value) -> void;

    private:
        std::array<bool, k_SettingsSectionCount> m_OpenSections{ true, true, false, false, false };
    };
}