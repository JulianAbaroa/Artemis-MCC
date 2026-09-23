export module UI.Format.System:Affordance;

import Egocentric.Affordance.Type;

export namespace UI::Format::System
{
	class AffordanceFormater
	{
	private:
		using Behaviour = Egocentric::Affordance::Type::Behavior;
		using Activation = Egocentric::Affordance::Type::Activation;

	public:
		AffordanceFormater() = default;
		~AffordanceFormater() = default;

		static auto BehaviorToString(Behaviour behaviour) -> const char*;
		static auto ActivationToString(Activation activation) -> const char*;
	};
}