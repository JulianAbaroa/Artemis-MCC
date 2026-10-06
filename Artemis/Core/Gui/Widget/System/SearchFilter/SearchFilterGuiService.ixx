export module Gui.Widget.System:SearchFilter;

import std;

export namespace Gui::Widget::System
{
    // Draws a search bar and matches text against what was typed, ignoring the case.
    class SearchFilterGuiService
    {
    public:
        SearchFilterGuiService() = default;
        ~SearchFilterGuiService() = default;

        // Draws the search bar at the full width of the window.
        // param hint: Text shown while the bar is empty.
        auto DrawSearchBar(const char* hint) -> void;

        auto GetQuery() const -> std::string_view;

        // return: True if the text contains the query, or if the query is empty.
        auto Matches(std::string_view text) const -> bool;

        auto IsActive() const -> bool
        {
            return m_Query[0] != '\0';
        }

    private:
        char m_Query[128]{};
        std::string m_LowerQuery{};

        auto RefreshLowerQuery() -> void;
    };
}