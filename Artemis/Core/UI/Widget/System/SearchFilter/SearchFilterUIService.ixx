export module UI.Widget.System:SearchFilter;

import std;

export namespace UI::Widget::System
{
	class SearchFilterUIService
	{
	public:
		SearchFilterUIService() = default;
		~SearchFilterUIService() = default;

		auto DrawSearchBar(const char* hint) -> void;

		auto GetQuery() const -> std::string_view;

		auto Matches(std::string_view text) const -> bool;

		auto IsActive() const -> bool
		{
			return m_Query[0] != '\0';
		}

	private:
		char m_Query[128] = "";
		std::string m_LowerQuery{};

		auto RefreshLowerQuery() -> void;
	};
}