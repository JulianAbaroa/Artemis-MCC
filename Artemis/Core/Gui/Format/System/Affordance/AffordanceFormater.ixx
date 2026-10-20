export module Gui.Format.System:Affordance;

import Egocentric.Affordance.Type;

export namespace Gui::Format::System
{
    // Converts the affordance enums to display text. Unlisted values give "Unknown".
    class AffordanceFormater
    {
    private:
        using AffordanceBehavior = Egocentric::Affordance::Type::AffordanceBehavior;
        using AffordanceActivation = Egocentric::Affordance::Type::AffordanceActivation;

    public:
        AffordanceFormater() = default;
        ~AffordanceFormater() = default;

        static auto BehaviorToString(AffordanceBehavior behavior) -> const char*;
        static auto ActivationToString(AffordanceActivation activation) -> const char*;
    };
}