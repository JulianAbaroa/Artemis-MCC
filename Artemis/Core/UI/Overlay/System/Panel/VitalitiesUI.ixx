export module UI.Overlay.System:Vitalities;

import Export.Tick.Type;
import Resolved.Vitality.State;
import std;

export namespace UI::Overlay::System
{
	class VitalitiesUI
	{
	private:
		using Tick = Export::Tick::Type::Tick;
		using VitalityStore = Resolved::Vitality::State::VitalityStore;

	public:
		VitalitiesUI() = default;
		~VitalitiesUI() = default;

		static auto Draw(const Tick& tick, std::uint32_t handle,
			const VitalityStore& vitalityStore) -> void;
	};
}