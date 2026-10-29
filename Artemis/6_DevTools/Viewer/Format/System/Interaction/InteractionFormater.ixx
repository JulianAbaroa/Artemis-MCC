export module DevTools.Viewer.Format.System:Interaction;

import Blam.Runtime.Interaction.Type;

export namespace DevTools::Viewer::Format::System
{
    // Converts the interaction enums to display text. Unlisted values give "Unknown".
    class InteractionFormater
    {
    private:
        using InteractionKind = Blam::Runtime::Interaction::Type::Alive::InteractionKind;
        using InteractionDetail = Blam::Runtime::Interaction::Type::Alive::InteractionDetail;

    public:
        InteractionFormater() = default;
        ~InteractionFormater() = default;

        static auto InteractionKindToString(InteractionKind kind) -> const char*;

        // The detail depends on the kind: a weapon action for GrabWeapon and a seat for vehicles.
        static auto InteractionDetailToString(InteractionKind kind,
            InteractionDetail detail) -> const char*;
    };
}