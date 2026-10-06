export module Gui.Format.System:Affordance;

import Egocentric.Affordance.Type;

export namespace Gui::Format::System
{
    // Converts the affordance enums to display text. Unlisted values give "Unknown".
    class AffordanceFormater
    {
    private:
        using Behavior = Egocentric::Affordance::Type::Behavior;
        using Activation = Egocentric::Affordance::Type::Activation;

    public:
        AffordanceFormater() = default;
        ~AffordanceFormater() = default;

        static auto BehaviorToString(Behavior behavior) -> const char*;
        static auto ActivationToString(Activation activation) -> const char*;
    };
}