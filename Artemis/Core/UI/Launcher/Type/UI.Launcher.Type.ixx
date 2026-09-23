export module UI.Launcher.Type;

import std;

export namespace UI::Launcher::Type
{
	enum class Tab : std::uint8_t
	{
		ObjectTable = 0,
		PlayerTable,
		Settings,
		MemoryScanner,
		Logs,

		Count
	};

	inline constexpr std::size_t k_TabCount = static_cast<std::size_t>(Tab::Count);

	constexpr auto ToIndex(Tab tab) -> std::size_t
	{
		return static_cast<std::size_t>(tab);
	}
}