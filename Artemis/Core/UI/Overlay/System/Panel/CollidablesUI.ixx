export module UI.Overlay.System:Collidables;

import Export.Tick.Type;
import std;

export namespace UI::Overlay::System
{
	class CollidablesUI
	{
	private:
		using Tick = Export::Tick::Type::Tick;

	public:
		CollidablesUI() = default;
		~CollidablesUI() = default;

		static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
	};
}