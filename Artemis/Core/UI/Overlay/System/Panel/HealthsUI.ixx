export module UI.Overlay.System:Vitalities;

import Export.Tick.Type;
import Resolved.Vitality.State;
import std;

export namespace UI::Overlay::System
{
	class HealthsUI
	{
	private:
		using Tick = Export::Tick::Type::Tick;
		using VitalityStore = Resolved::Vitality::State::VitalityStore;

	public:
		HealthsUI() = default;
		~HealthsUI() = default;

		static auto Draw(const Tick& tick, std::uint32_t handle,
			const VitalityStore& vitalityStore) -> void;
	};
}