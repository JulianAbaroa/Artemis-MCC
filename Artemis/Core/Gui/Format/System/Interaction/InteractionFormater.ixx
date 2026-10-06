export module Gui.Format.System:Interaction;

import Tables.Interaction.Type;

export namespace Gui::Format::System
{
    // Converts the interaction enums to display text. Unlisted values give "Unknown".
    class InteractionFormater
    {
    private:
        using InteractionKind = Tables::Interaction::Type::Alive::Kind;
        using InteractionDetail = Tables::Interaction::Type::Alive::Detail;

    public:
        InteractionFormater() = default;
        ~InteractionFormater() = default;

        static auto InteractionKindToString(InteractionKind kind) -> const char*;

        // The detail depends on the kind: a weapon action for GrabWeapon and a seat for vehicles.
        static auto InteractionDetailToString(InteractionKind kind,
            InteractionDetail detail) -> const char*;
    };
}