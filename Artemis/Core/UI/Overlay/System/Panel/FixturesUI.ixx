export module UI.Overlay.System:Fixtures;

import Export.Tick.Type;
import std;

export namespace UI::Overlay::System
{
	class FixturesUI
	{
	private:
		using Tick = Export::Tick::Type::Tick;

	public:
		FixturesUI() = default;
		~FixturesUI() = default;

		static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
	};
}