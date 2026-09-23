export module UI.Overlay.System:Affordances;

import Export.Tick.Type;
import std;

export namespace UI::Overlay::System
{
	class AffordancesUI
	{
	private:
		using Tick = Export::Tick::Type::Tick;

	public:
		AffordancesUI() = default;
		~AffordancesUI() = default;

		static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
	};
}