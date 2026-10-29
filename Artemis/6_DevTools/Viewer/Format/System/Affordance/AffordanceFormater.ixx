export module DevTools.Viewer.Format.System:Affordance;

import Views.Affordance.Type;

export namespace DevTools::Viewer::Format::System
{
    // Converts the affordance enums to display text. Unlisted values give "Unknown".
    class AffordanceFormater
    {
    private:
        using AffordanceBehavior = Views::Affordance::Type::AffordanceBehavior;
        using AffordanceActivation = Views::Affordance::Type::AffordanceActivation;

    public:
        AffordanceFormater() = default;
        ~AffordanceFormater() = default;

        static auto BehaviorToString(AffordanceBehavior behavior) -> const char*;
        static auto ActivationToString(AffordanceActivation activation) -> const char*;
    };
}