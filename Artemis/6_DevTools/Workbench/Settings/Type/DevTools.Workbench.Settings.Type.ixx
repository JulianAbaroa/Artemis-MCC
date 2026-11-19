export module DevTools.Workbench.Settings.Type;

import std;

export namespace DevTools::Workbench::Settings::Type
{
    // Collapsible sections of the settings tab, in the order they are drawn.
    enum class SettingsSection : std::uint8_t
    {
        Data = 0,
        Interface,
        Hotkeys,
        Folders,

        Count
    };

    inline constexpr std::size_t k_SettingsSectionCount{ static_cast<std::size_t>(SettingsSection::Count) };

    // Titles of the section headers, in the order of SettingsSection.
    inline constexpr std::array<const char*, k_SettingsSectionCount> k_SettingsSectionTitles
    {
        "Data",
        "Interface",
        "Hotkeys",
        "Folders",
    };

    // Keys of the sections in the preferences file, in the order of SettingsSection.
    inline constexpr std::array<std::string_view, k_SettingsSectionCount> k_SettingsSectionKeys
    {
        "Data",
        "Interface",
        "Hotkeys",
        "Folders",
    };
}