export module UI.Launcher.Type;

import std;

export namespace UI::Launcher::Type
{
    // Tabs of the launcher window.
    enum class Tab : std::uint8_t
    {
        ObjectTable = 0,
        PlayerTable,
        DefinitionsInspector,
        Settings,
        MemoryScanner,
        Logs,

        Count
    };

    inline constexpr std::size_t k_TabCount{ static_cast<std::size_t>(Tab::Count) };

    // Content callback of a tab window.
    using TabContent = std::function<void()>;

    // Converts a tab to its array index.
    constexpr auto ToIndex(Tab tab) -> std::size_t
    {
        return static_cast<std::size_t>(tab);
    }
}