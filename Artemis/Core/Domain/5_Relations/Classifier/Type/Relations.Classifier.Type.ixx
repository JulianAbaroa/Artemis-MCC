export module Relations.Classifier.Type;

import Common.Semantic.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Group;
    using Common::Semantic::Type::Binding;
    using Common::Semantic::Type::Capability;
    using Resolved::Definitions::Type::Object::ObjectKind;
}

export namespace Relations::Classifier::Type
{
    // Legacy role of an object.
    // note: Kept until B3e, while its consumers move to Function, Binding and the capabilities (Ignore/Semantizacion/08).
    enum class Role : std::uint8_t
    {
        Unknown = 0,
        None,

        // --- bipd ---

        Biped,

        // --- vehi ---

        Vehicle,
        VehiclePart,

        // --- weap ---

        WeaponPickup,
        WeaponEquipped,

        ObjectivePickup,
        ObjectiveEquipped,

        // --- eqip ---

        ArmorAbilityPickup,
        ArmorAbilityEquipped,
        GrenadePickup,
        AmmoPickup,
        Powerup,

        // --- proj ---

        Projectile,

        // --- scen ---

        Spawn,
        Boundary,
        PortableShield,
        SceneryObstacle,

        // --- mach ---

        DeviceMachine,

        // --- bloc ---

        ObjectiveSpawn,
        ObjectiveZone,
        Teleporter,
        Explosive,
        Pallet,
        Lift,
        Shield,
        CrateObstacle,

        // --- ctrl ---

        HealthStation,
    };

    // Rule that decided the function of a tag (Ignore/Semantizacion/05, section 3).
    enum class Source : std::uint8_t
    {
        // No rule matched. The function is Unknown.
        None = 0,

        // Hand label of Data/Tags/ObjectTags.xml.
        Catalog,

        // Multiplayer object kind of the tag.
        MultiplayerKind,

        // Default of the object kind.
        KindDefault,
    };

    // Classification shared by every object of one tag. Function and subtype never change with the state of the object (M2).
    struct TagClassification
    {
        ::Function Function{ ::Function::Unknown };

        // Value of the subtype enum of the function (Subtype::<Function>). 0 is None.
        std::uint8_t Subtype{};

        Type::Source Source{ Type::Source::None };

        // Function the multiplayer object kind of the tag gives, Unknown if none. Kept to audit the catalog against the engine.
        ::Function MultiplayerFunction{ ::Function::Unknown };

        // Capabilities that follow from the tag and the function only.
        Capability Capabilities{ Capability::None };
    };

    // Classification of one live object.
    struct Classified
    {
        std::uint32_t Handle{};

        // note: Legacy. Removed in B3e.
        Type::Role Role{ Type::Role::Unknown };

        ::ObjectKind Kind{ ::ObjectKind::Invalid };
        ::Function Function{ ::Function::Unknown };
        std::uint8_t Subtype{};
        ::Group Group{ ::Group::Unknown };
        Type::Source Source{ Type::Source::None };

        // Link to the parent and the parent handle, or 0xFFFFFFFF when Free.
        ::Binding Binding{ ::Binding::Free };
        std::uint32_t BoundTo{ 0xFFFFFFFF };

        // Static capabilities of the tag, plus HasZone from the multiplayer block of the object.
        // note: The dynamic ones (IsDynamic, IsPickable, IsDestructible...) come with Entities in B4a.
        Capability Capabilities{ Capability::None };
    };
}