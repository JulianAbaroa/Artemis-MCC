export module Gui.Format.System:Object;

import Tables.Object.Type;

export namespace Gui::Format::System
{
    // Converts the object enums to display text. Unlisted values give "Unknown".
    class ObjectFormater
    {
    private:
        using WeaponAction = Tables::Object::Type::Weapon::WeaponAction;
        using VehicleKind = Tables::Object::Type::Vehicle::VehicleKind;
        using ZoomLevel = Tables::Object::Type::Biped::ZoomLevel;
        using CrateKind = Tables::Object::Type::Crate::CrateKind;
        using SceneryKind = Tables::Object::Type::Scenery::SceneryKind;

    public:
        ObjectFormater() = default;
        ~ObjectFormater() = default;

        static auto WeaponActionToString(WeaponAction action) -> const char*;
        static auto VehicleKindToString(VehicleKind kind) -> const char*;
        static auto ZoomLevelToString(ZoomLevel zoom) -> const char*;
        static auto CrateKindToString(CrateKind kind) -> const char*;
        static auto SceneryKindToString(SceneryKind kind) -> const char*;
    };
}