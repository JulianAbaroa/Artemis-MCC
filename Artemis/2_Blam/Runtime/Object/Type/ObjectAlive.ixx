export module Tables.Object.Type:Alive;

import :Profile;
import :Equipment;
import :Vehicle;
import :Weapon;
import :Biped;
import :Crate;
import :Scenery;
import :Projectile;
import :Multiplayer;
import :Placement;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Tables::Object::Type::Profile::Profile;
    using Tables::Object::Type::Equipment::Equipment;
    using Tables::Object::Type::Vehicle::Vehicle;
    using Tables::Object::Type::Weapon::Weapon;
    using Tables::Object::Type::Biped::Biped;
    using Tables::Object::Type::Crate::Crate;
    using Tables::Object::Type::Scenery::Scenery;
    using Tables::Object::Type::Projectile::Projectile;
    using Tables::Object::Type::Multiplayer::Multiplayer;
    using Tables::Object::Type::Placement::Placement;
}

export namespace Tables::Object::Type::Alive
{
    // Data of the kind of the object. Monostate for the kinds without specific data.
    struct Specific : std::variant<std::monostate, Equipment::Equipment, Vehicle::Vehicle, Weapon::Weapon, Biped::Biped, Crate::Crate,
        Scenery::Scenery, Projectile::Projectile>
    {
        using variant::variant;
    };

    struct AliveObject
    {
        std::uint32_t DatumIndex{};

        std::string FourCC{};
        std::string TagName{};
        std::uint16_t HlmtVariant{};

        std::uint32_t Handle{};
        std::uint32_t NextSiblingHandle{};
        std::uint32_t ChildHandle{};
        std::uint32_t ParentHandle{};

        // note: Center of the bounding sphere, world space (offset 0x20). Not the origin of the object (D27).
        Vec3 BoundingCenter{};
        // note: object origin (offset 0x44), local to the parent if parented.
        Vec3 Origin{};
        // note: origin resolved to world space through the parent chain.
        // Zone shapes (Top/Bottom) are measured from here, not from BoundingCenter.
        Vec3 WorldOrigin{};
        Vec3 Forward{};
        Vec3 Up{};

        Vec3 LinearVelocity{};
        Vec3 AngularVelocity{};

        float BoundingRadius{};
        float DamageReceived{};

        ::Profile Profile{};

        // note: Only objects that have the multiplayer block.
        std::optional<::Multiplayer> Multiplayer{};
        // note: Only objects placed by the map variant (obje 0x1C). Holds the Forge settings, Physics included.
        std::optional<::Placement> Placement{};
        Specific Specific{};
    };

    // Live objects, by handle.
    struct ObjectTable : std::unordered_map<std::uint32_t, AliveObject>
    {
        using unordered_map::unordered_map;
    };
}