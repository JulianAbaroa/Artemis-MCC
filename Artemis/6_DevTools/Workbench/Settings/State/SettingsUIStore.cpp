module DevTools.Workbench.Settings.State;

namespace
{
    using DevTools::Workbench::Settings::Type::k_SettingsSectionKeys;
}

namespace DevTools::Workbench::Settings::State
{
    auto SettingsUIStore::IsSectionOpen(SettingsSection section) const -> bool
    {
        return m_OpenSections[static_cast<std::size_t>(section)];
    }

    auto SettingsUIStore::SetSectionOpen(SettingsSection section, bool isOpen) -> void
    {
        m_OpenSections[static_cast<std::size_t>(section)] = isOpen;
    }

    auto SettingsUIStore::Save(std::ostream& stream) const -> void
    {
        for (std::size_t section{ 0 }; section < m_OpenSections.size(); ++section)
        {
            stream << k_SettingsPreferencePrefix << k_SettingsSectionKeys[section] << '=' << (m_OpenSections[section] ? 1 : 0) << '\n';
        }
    }

    auto SettingsUIStore::Load(std::string_view key, std::string_view value) -> void
    {
        if (value != "0" && value != "1") return;

        for (std::size_t section{ 0 }; section < m_OpenSections.size(); ++section)
        {
            if (key == k_SettingsSectionKeys[section]) m_OpenSections[section] = value == "1";
        }
    }
}