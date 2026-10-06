export module Viewer.Options.Type;

import std;

export namespace Viewer::Options::Type
{
    // Section of the options window.
    enum class Group : std::uint8_t
    {
        General,
        Players,
        Objects,
        Vehicles,
        Health,
        Interaction,
        Fixtures,

        Count
    };

    // On/off option. The order must follow k_Flags.
    enum class Flag : std::uint8_t
    {
        Labels,

        PlayerGamertag,
        PlayerTeamColor,
        PlayerDistance,
        PlayerWeaponEquipped,
        PlayerWeaponPrimary,
        PlayerWeaponSecondary,
        PlayerWeaponHeld,
        PlayerSelf,

        ObjectSelectedOnly,
        ObjectVehicles,
        ObjectBipeds,
        ObjectWeapons,
        ObjectEquipment,
        ObjectProjectiles,
        ObjectOther,
        ObjectTagName,
        ObjectRole,
        ObjectHandle,
        ObjectDistance,
        ObjectLinearVelocity,
        ObjectVelocityArrow,
        ObjectAngularVelocity,
        ObjectDamage,
        ObjectParent,

        VehicleSeatSummary,
        VehicleSeatMarkers,
        VehicleHijackerSlots,

        HealthBars,
        HealthSelectedOnly,
        HealthBarValue,
        HealthBarTags,
        HealthShieldSections,
        HealthIncludeCenter,
        HealthTintSpheres,

        InteractionObject,
        InteractionMelee,
        InteractionAim,
        InteractionAimHitPoint,
        InteractionLink,
        InteractionAffordanceDetails,
        AffordanceAll,

        FixtureTeleportLabels,
        FixtureTeleportLinks,
        FixtureLiftLabels,
        FixtureLiftArrows,
        FixtureShieldLabels,
        FixtureShieldArrows,
        FixtureObjectives,
        FixtureDestructibles,
        FixtureSpawns,
        FixtureShieldTranslucent,

        Count
    };

    // Numeric option with a range. The order must follow k_Scalars.
    enum class Scalar : std::uint8_t
    {
        LabelMaxDistance,
        LabelMaxCount,
        LabelTextScale,
        LabelBackgroundAlpha,

        ObjectArrowLength,

        HealthBarWidth,

        FixtureArrowLength,
        ShieldOpacity,

        Count
    };

    struct GroupInfo
    {
        Group Id{};
        const char* Label{};
        bool IsOpenByDefault{};
    };

    // Key is the name saved in the preferences file, so it must not change.
    struct FlagInfo
    {
        Flag Id{};
        Group Group{};
        const char* Key{};
        const char* Label{};
        const char* Tooltip{};
        bool Default{};
    };

    // Key is the name saved in the preferences file, so it must not change.
    // Format is the printf format used to show the value.
    struct ScalarInfo
    {
        Scalar Id{};
        Group Group{};
        const char* Key{};
        const char* Label{};
        const char* Tooltip{};
        float Default{};
        float Min{};
        float Max{};
        const char* Format{};
    };

    inline constexpr std::size_t k_GroupCount{ static_cast<std::size_t>(Group::Count) };
    inline constexpr std::size_t k_FlagCount{ static_cast<std::size_t>(Flag::Count) };
    inline constexpr std::size_t k_ScalarCount{ static_cast<std::size_t>(Scalar::Count) };

    inline constexpr std::array<GroupInfo, k_GroupCount> k_Groups
    { {
        { Group::General, "General", true },
        { Group::Players, "Players", true },
        { Group::Objects, "Objects", false },
        { Group::Vehicles, "Vehicles", false },
        { Group::Health, "Health", true },
        { Group::Interaction, "Interaction", true },
        { Group::Fixtures, "Fixtures", true },
    } };

    inline constexpr std::array<FlagInfo, k_FlagCount> k_Flags
    { {
        { Flag::Labels, Group::General, "Labels", "Show labels",
            "Master switch for every 3D label drawn over the map.", true },

        { Flag::PlayerGamertag, Group::Players, "PlayerGamertag", "Gamertag",
            "Shows the gamertag above each player.", true },

        { Flag::PlayerTeamColor, Group::Players, "PlayerTeamColor", "Team color",
            "Tints the player label with the team color.", true },

        { Flag::PlayerDistance, Group::Players, "PlayerDistance", "Distance to local player",
            "Shows the distance in world units from the local player to each player. Shows n/a while the local player is dead.", false },

        { Flag::PlayerWeaponEquipped, Group::Players, "PlayerWeaponEquipped", "Equipped weapon",
            "Tag of the first weapon attached to the player's biped, which is the weapon in use.", true },

        { Flag::PlayerWeaponPrimary, Group::Players, "PlayerWeaponPrimary", "Primary weapon (player table)",
            "Tag of the weapon referenced by the player's primary weapon slot.", false },

        { Flag::PlayerWeaponSecondary, Group::Players, "PlayerWeaponSecondary", "Secondary weapon (player table)",
            "Tag of the weapon referenced by the player's secondary weapon slot.", false },

        { Flag::PlayerWeaponHeld, Group::Players, "PlayerWeaponHeld", "Weapons attached to biped",
            "Tags of the weapon objects whose parent is the player's biped. Useful to compare against the player table slots.", false },

        { Flag::PlayerSelf, Group::Players, "PlayerSelf", "Include local player",
            "Also labels the local player.", false },

        { Flag::ObjectSelectedOnly, Group::Objects, "ObjectSelectedOnly", "Selected object only",
            "Only the selected object gets a label. Turn it off to label every enabled category.", true },

        { Flag::ObjectVehicles, Group::Objects, "ObjectVehicles", "Category: vehicles",
            "Labels vehicles when not restricted to the selection.", true },

        { Flag::ObjectBipeds, Group::Objects, "ObjectBipeds", "Category: bipeds",
            "Labels bipeds when not restricted to the selection.", false },

        { Flag::ObjectWeapons, Group::Objects, "ObjectWeapons", "Category: weapon pickups",
            "Labels weapon and objective pickups on the ground.", false },

        { Flag::ObjectEquipment, Group::Objects, "ObjectEquipment", "Category: equipment pickups",
            "Labels armor abilities, grenades, ammo and powerups on the ground.", false },

        { Flag::ObjectProjectiles, Group::Objects, "ObjectProjectiles", "Category: projectiles",
            "Labels projectiles in flight.", false },

        { Flag::ObjectOther, Group::Objects, "ObjectOther", "Category: other",
            "Labels every remaining object role.", false },

        { Flag::ObjectTagName, Group::Objects, "ObjectTagName", "Tag name",
            "Shows the short tag name of the object.", true },

        { Flag::ObjectRole, Group::Objects, "ObjectRole", "Role",
            "Shows the role assigned by the classifier.", false },

        { Flag::ObjectHandle, Group::Objects, "ObjectHandle", "Handle",
            "Shows the object handle in hexadecimal.", false },

        { Flag::ObjectDistance, Group::Objects, "ObjectDistance", "Distance to local player",
            "Shows the distance in world units from the local player. Shows n/a while the local player is dead.", false },

        { Flag::ObjectLinearVelocity, Group::Objects, "ObjectLinearVelocity", "Linear velocity",
            "Shows the speed of the object in world units per second.", true },

        { Flag::ObjectVelocityArrow, Group::Objects, "ObjectVelocityArrow", "Velocity arrow",
            "Draws an arrow pointing where the object is moving, which can differ from where it faces. The arrow has a fixed length set by the slider.", false },

        { Flag::ObjectAngularVelocity, Group::Objects, "ObjectAngularVelocity", "Angular velocity",
            "Shows the magnitude of the angular velocity as stored by the engine.", false },

        { Flag::ObjectDamage, Group::Objects, "ObjectDamage", "Damage received",
            "Shows the accumulated damage value of the object.", false },

        { Flag::ObjectParent, Group::Objects, "ObjectParent", "Parent handle",
            "Shows the handle of the parent object, when there is one.", false },

        { Flag::VehicleSeatSummary, Group::Vehicles, "VehicleSeatSummary", "Seat summary on vehicle label",
            "Adds one row per seat to the vehicle label with its state and occupant.", true },

        { Flag::VehicleSeatMarkers, Group::Vehicles, "VehicleSeatMarkers", "Seat markers",
            "Draws a label at the world position of each seat with state and distance to the local player.", false },

        { Flag::VehicleHijackerSlots, Group::Vehicles, "VehicleHijackerSlots", "Include hijacker slots",
            "Also lists the hijacker slots of each vehicle.", false },

        { Flag::HealthBars, Group::Health, "HealthBars", "Section health bars",
            "Draws a bar over each damage section sphere showing its remaining vitality.", true },

        { Flag::HealthSelectedOnly, Group::Health, "HealthSelectedOnly", "Selected object only",
            "Only the selected object gets health bars. Turn it off to draw bars for every object with health.", true },

        { Flag::HealthBarValue, Group::Health, "HealthBarValue", "Show value",
            "Writes the numeric vitality inside each bar.", false },

        { Flag::HealthBarTags, Group::Health, "HealthBarTags", "Show section tags",
            "Marks sections: K kills the object, H headshot, D destroys it, S shield.", true },

        { Flag::HealthShieldSections, Group::Health, "HealthShieldSections", "Include shield sections",
            "Also draws bars for shield sections.", true },

        { Flag::HealthIncludeCenter, Group::Health, "HealthIncludeCenter", "Include object-center anchors",
            "Also draws bars for sections that only have an imprecise object-center anchor.", false },

        { Flag::HealthTintSpheres, Group::Health, "HealthTintSpheres", "Tint aim spheres by vitality",
            "Colors each aim sphere from red to green according to its vitality, instead of its anchor source.", true },

        { Flag::InteractionObject, Group::Interaction, "InteractionObject", "Engine-highlighted object",
            "Marks the object the game engine currently offers to interact with, with the interaction kind and detail.", true },

        { Flag::InteractionMelee, Group::Interaction, "InteractionMelee", "Melee target",
            "Marks the target the engine reports as available for melee.", true },

        { Flag::InteractionAim, Group::Interaction, "InteractionAim", "Aim target",
            "Marks the object the local player is aiming at, with the target slot and model part.", true },

        { Flag::InteractionAimHitPoint, Group::Interaction, "InteractionAimHitPoint", "Aim hit point",
            "Marks where the local player's camera ray lands, with its distance.", true },

        { Flag::InteractionLink, Group::Interaction, "InteractionLink", "Arrow to target",
            "Draws an arrow from the local player to each marked interaction target.", true },

        { Flag::InteractionAffordanceDetails, Group::Interaction, "InteractionAffordanceDetails", "Affordance details on target",
            "Adds the behaviors and activation of the affordance matching the engine-highlighted object.", true },

        { Flag::AffordanceAll, Group::Interaction, "AffordanceAll", "Label every affordance",
            "Labels every object the local player can currently interact with, with role, behaviors, activation and distance.", false },

        { Flag::FixtureTeleportLabels, Group::Fixtures, "FixtureTeleportLabels", "Teleport labels",
            "Labels each teleporter with its kind (sender, receiver, two-way), channel, zone and what it lets through.", true },

        { Flag::FixtureTeleportLinks, Group::Fixtures, "FixtureTeleportLinks", "Teleport links",
            "Draws dashed arrows from each teleporter to its destinations.", true },

        { Flag::FixtureLiftLabels, Group::Fixtures, "FixtureLiftLabels", "Lift labels",
            "Labels each lift with its angle and force.", true },

        { Flag::FixtureLiftArrows, Group::Fixtures, "FixtureLiftArrows", "Lift launch arrows",
            "Draws an arrow along the launch direction of each lift.", true },

        { Flag::FixtureShieldLabels, Group::Fixtures, "FixtureShieldLabels", "Shield labels",
            "Labels each shield door or wall with its kind (one-way, two-way, blocker).", true },

        { Flag::FixtureShieldArrows, Group::Fixtures, "FixtureShieldArrows", "Shield direction arrows",
            "Draws an arrow along the block direction of one-way shields.", true },

        { Flag::FixtureObjectives, Group::Fixtures, "FixtureObjectives", "Objectives",
            "Labels objectives (flags, bombs) and objective spawn zones with team and carrier.", true },

        { Flag::FixtureDestructibles, Group::Fixtures, "FixtureDestructibles", "Destructibles",
            "Labels pallets, explosives and portable shields with kind and health.", false },

        { Flag::FixtureSpawns, Group::Fixtures, "FixtureSpawns", "Spawn points",
            "Labels player spawn points with kind and team.", false },

        { Flag::FixtureShieldTranslucent, Group::Fixtures, "FixtureShieldTranslucent", "Translucent shields",
            "Draws shield doors, walls and drop shields translucent so the geometry behind them stays visible.", true },
    } };

    inline constexpr std::array<ScalarInfo, k_ScalarCount> k_Scalars
    { {
        { Scalar::LabelMaxDistance, Group::General, "LabelMaxDistance", "Max label distance",
            "Labels farther than this from the camera are hidden.", 150.0f, 5.0f, 1000.0f, "%.0f" },

        { Scalar::LabelMaxCount, Group::General, "LabelMaxCount", "Max labels",
            "Upper bound of labels drawn per frame, nearest first.", 64.0f, 4.0f, 256.0f, "%.0f" },

        { Scalar::LabelTextScale, Group::General, "LabelTextScale", "Text scale",
            "Multiplier applied to the label text size.", 1.0f, 0.5f, 2.0f, "%.2f" },

        { Scalar::LabelBackgroundAlpha, Group::General, "LabelBackgroundAlpha", "Background opacity",
            "Opacity of the panel behind each label.", 0.45f, 0.0f, 1.0f, "%.2f" },

        { Scalar::ObjectArrowLength, Group::Objects, "ObjectArrowLength", "Velocity arrow length (wu)",
            "Length of the velocity arrow in world units. The arrow only shows the direction.", 1.5f, 0.25f, 10.0f, "%.2f" },

        { Scalar::HealthBarWidth, Group::Health, "HealthBarWidth", "Bar width (px)",
            "Width of each section health bar before text scaling.", 46.0f, 20.0f, 120.0f, "%.0f" },

        { Scalar::FixtureArrowLength, Group::Fixtures, "FixtureArrowLength", "Fixture arrow length (wu)",
            "Length of lift and shield direction arrows in world units.", 3.0f, 0.5f, 15.0f, "%.1f" },

        { Scalar::ShieldOpacity, Group::Fixtures, "ShieldOpacity", "Shield opacity",
            "Opacity of translucent shields. Lower values are more see-through.", 0.30f, 0.05f, 1.0f, "%.2f" },
    } };

    // return: True if every table has its entries in the order of its enum, so the enum values can index them.
    consteval auto AreTablesOrdered() -> bool
    {
        for (std::size_t i = 0; i < k_FlagCount; ++i)
        {
            if (static_cast<std::size_t>(k_Flags[i].Id) != i) return false;
        }

        for (std::size_t i = 0; i < k_ScalarCount; ++i)
        {
            if (static_cast<std::size_t>(k_Scalars[i].Id) != i) return false;
        }

        for (std::size_t i = 0; i < k_GroupCount; ++i)
        {
            if (static_cast<std::size_t>(k_Groups[i].Id) != i) return false;
        }

        return true;
    }

    static_assert(AreTablesOrdered(), "Viewer option tables must follow enum order.");
}