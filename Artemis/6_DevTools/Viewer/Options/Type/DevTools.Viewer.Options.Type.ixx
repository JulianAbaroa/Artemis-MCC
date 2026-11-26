export module DevTools.Viewer.Options.Type;

import std;

export namespace DevTools::Viewer::Options::Type
{
    // Section of the Viewer tab an option is drawn in. The layers are drawn apart, one row each (Layer).
    enum class Group : std::uint8_t
    {
        Camera,
        Layers,
        Development,
        Director,

        Count
    };

    // Family of options that is switched together by a Level. The order must follow k_Layers.
    enum class Layer : std::uint8_t
    {
        View,
        Players,
        Objects,
        Fixtures,
        World,
        Health,
        Card,
        Obs,

        Count
    };

    // How much a layer shows: nothing, the base set, or everything.
    enum class Level : std::uint8_t
    {
        Off,
        Minimal,
        Full
    };

    // On/off option. The order must follow k_Flags.
    enum class Flag : std::uint8_t
    {
        Labels,

        PlayerGamertag,
        PlayerTeamColor,
        PlayerDistance,
        PlayerVitality,
        PlayerWeaponEquipped,
        PlayerWeaponPrimary,
        PlayerWeaponSecondary,
        PlayerWeaponHeld,
        PlayerSelf,
        PlayerView,

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
        AimSpheres,

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
        FixtureSpawns,
        FixtureShieldTranslucent,

        LimitKillVolumes,
        LimitSafeVolumes,
        LimitSoftCeilings,

        OverlayWireframe,
        HideGameHud,
        HideFirstPerson,
        WatchPov,
        PossessBot,
        DirectorAutoPlay,
        AgentStrip,
        RosterAgentsOnly,
        DrawOverlay,

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

        LimitOpacity,

        SpeedCustom,
        FovNormal,
        FovZoom,

        SceneColorScale,
        DirectorDegreesPerSecond,

        Count
    };

    // What a layer is for. A Debug layer is part of the debug overlay and follows its master switch (Flag::DrawOverlay). A Presentation layer is made to be
    // shown (in a video, to the audience) and ignores that switch: it only follows its own level.
    enum class LayerKind : std::uint8_t
    {
        Debug,
        Presentation
    };

    // Key is the name saved in the preferences file (with the Viewer_ prefix), so it must not change.
    struct LayerInfo
    {
        Layer Id{};
        const char* Key{};
        const char* Label{};
        const char* Tooltip{};
        Level Default{ Level::Minimal };
        LayerKind Kind{ LayerKind::Debug };
    };

    // Key is the name saved in the preferences file, so it must not change.
    // A flag with an Owner is not saved: it is derived from the level of its layer (IsFlagOn). Default is the value it had before the layers,
    // which the level Minimal of every layer reproduces. MinLevel is Minimal when that default is on and Full when it is off.
    // IsInverted flags are on only at the level Minimal (the selection-only flags: Full shows every object).
    // Owner is Layer::Count for a flag that belongs to no layer, which is saved by its own key.
    struct FlagInfo
    {
        Flag Id{};
        Group Group{};
        Layer Owner{ Layer::Count };
        Level MinLevel{ Level::Minimal };
        const char* Key{};
        const char* Label{};
        const char* Tooltip{};
        bool Default{};
        bool IsInverted{};
    };

    // Key is the name saved in the preferences file, so it must not change.
    // Format is the printf format used to show the value.
    struct ScalarInfo
    {
        Scalar Id{};
        Group Group{};
        Layer Owner{ Layer::Count };   // the layer whose row shows it folded below, Layer::Count for none
        const char* Key{};
        const char* Label{};
        const char* Tooltip{};
        float Default{};
        float Min{};
        float Max{};
        const char* Format{};
        bool IsLogarithmic{};   // the slider is logarithmic and the value is saved with significant digits (a tiny value must not be saved as 0.000)
    };

    inline constexpr std::size_t k_LayerCount{ static_cast<std::size_t>(Layer::Count) };
    inline constexpr std::size_t k_FlagCount{ static_cast<std::size_t>(Flag::Count) };
    inline constexpr std::size_t k_ScalarCount{ static_cast<std::size_t>(Scalar::Count) };

    inline constexpr std::array<LayerInfo, k_LayerCount> k_Layers
    { {
        { Layer::View, "LayerView", "View",
            "What the watched player knows and what the engine offers to interact with." },
        { Layer::Players, "LayerPlayers", "Players",
            "Labels and markers of the players." },
        { Layer::Objects, "LayerObjects", "Objects",
            "Labels of the objects and the seats of the vehicles." },
        { Layer::Fixtures, "LayerFixtures", "Fixtures",
            "Teleporters, lifts, shields, objectives and spawn points." },
        { Layer::World, "LayerWorld", "World",
            "The collision wireframe and the volumes and surfaces that limit the map." },
        { Layer::Health, "LayerHealth", "Health",
            "Health bars over the damage sections." },
        { Layer::Card, "LayerCard", "Agent card",
            "Card of the selected player at the bottom left of the frame, with no flags of its own.\nMinimal: name, health and shield bars, weapon in the hands.\nFull adds: other weapon, grenades, armor ability and energy, powerup, flag, zoom.", Level::Minimal, LayerKind::Presentation },
        { Layer::Obs, "LayerObs", "Agent observation",
            "The two arrows that the policy of each Artemis agent receives in its observation, over the game, with no flags of its own.\nMinimal: from its feet to the enemy flag (in the color of the rival team) and to its own stand (in the color of its team).\nFull adds: the distance of each arrow in world units, near its tip.", Level::Off, LayerKind::Presentation },
    } };

    inline constexpr std::array<FlagInfo, k_FlagCount> k_Flags
    { {
        { Flag::Labels, Group::Layers, Layer::Count, Level::Minimal, "Labels", "Show labels",
            "Master switch for every 3D label drawn over the map.", true },

        { Flag::PlayerGamertag, Group::Layers, Layer::Players, Level::Minimal, "PlayerGamertag", "Gamertag",
            "Shows the gamertag above each player.", true },

        { Flag::PlayerTeamColor, Group::Layers, Layer::Players, Level::Minimal, "PlayerTeamColor", "Team color",
            "Tints the player label with the team color.", true },

        { Flag::PlayerDistance, Group::Layers, Layer::Players, Level::Full, "PlayerDistance", "Distance to local player",
            "Shows the distance in world units from the local player to each player. Shows n/a while the local player is dead.", false },

        { Flag::PlayerVitality, Group::Layers, Layer::Players, Level::Full, "PlayerVitality", "Shield and health",
            "Shield (first shield section) and health (life section) of the player's biped, from 0 to 1.", false },

        { Flag::PlayerWeaponEquipped, Group::Layers, Layer::Players, Level::Minimal, "PlayerWeaponEquipped", "Equipped weapon",
            "Tag of the weapon in the hands of the player's biped (loadout). None in a vehicle.", true },

        { Flag::PlayerWeaponPrimary, Group::Layers, Layer::Players, Level::Full, "PlayerWeaponPrimary", "Primary weapon",
            "Tag of the weapon in the primary slot of the loadout.", false },

        { Flag::PlayerWeaponSecondary, Group::Layers, Layer::Players, Level::Full, "PlayerWeaponSecondary", "Secondary weapon",
            "Tag of the weapon in the secondary slot of the loadout.", false },

        { Flag::PlayerWeaponHeld, Group::Layers, Layer::Players, Level::Full, "PlayerWeaponHeld", "Weapons carried by biped",
            "Tags of the weapon entities carried by the player's biped. Useful to compare against the loadout slots.", false },

        { Flag::PlayerSelf, Group::Layers, Layer::Players, Level::Full, "PlayerSelf", "Include local player",
            "Also labels the local player.", false },

        { Flag::PlayerView, Group::Layers, Layer::View, Level::Minimal, "PlayerView", "View of the watched player",
            "Draws what the watched player knows (its View): the cone of sight, a line to each player it sees, its allies, the flags, and in grey the players it does not know.", true },

        { Flag::ObjectSelectedOnly, Group::Layers, Layer::Objects, Level::Minimal, "ObjectSelectedOnly", "Selected object only",
            "Only the selected object gets a label. Turn it off to label every enabled category.", true, true },

        { Flag::ObjectVehicles, Group::Layers, Layer::Objects, Level::Minimal, "ObjectVehicles", "Category: vehicles",
            "Labels vehicles when not restricted to the selection.", true },

        { Flag::ObjectBipeds, Group::Layers, Layer::Objects, Level::Full, "ObjectBipeds", "Category: bipeds",
            "Labels bipeds when not restricted to the selection.", false },

        { Flag::ObjectWeapons, Group::Layers, Layer::Objects, Level::Full, "ObjectWeapons", "Category: weapon pickups",
            "Labels weapon and objective pickups on the ground.", false },

        { Flag::ObjectEquipment, Group::Layers, Layer::Objects, Level::Full, "ObjectEquipment", "Category: equipment pickups",
            "Labels armor abilities, grenades, ammo and powerups on the ground.", false },

        { Flag::ObjectProjectiles, Group::Layers, Layer::Objects, Level::Full, "ObjectProjectiles", "Category: projectiles",
            "Labels projectiles in flight.", false },

        { Flag::ObjectOther, Group::Layers, Layer::Objects, Level::Full, "ObjectOther", "Category: other",
            "Labels every remaining object role.", false },

        { Flag::ObjectTagName, Group::Layers, Layer::Objects, Level::Minimal, "ObjectTagName", "Tag name",
            "Shows the short tag name of the object.", true },

        { Flag::ObjectRole, Group::Layers, Layer::Objects, Level::Full, "ObjectRole", "Function",
            "Shows the function, the subtype and the binding the classifier assigned.", false },

        { Flag::ObjectHandle, Group::Layers, Layer::Objects, Level::Full, "ObjectHandle", "Handle",
            "Shows the object handle in hexadecimal.", false },

        { Flag::ObjectDistance, Group::Layers, Layer::Objects, Level::Full, "ObjectDistance", "Distance to local player",
            "Shows the distance in world units from the local player. Shows n/a while the local player is dead.", false },

        { Flag::ObjectLinearVelocity, Group::Layers, Layer::Objects, Level::Minimal, "ObjectLinearVelocity", "Linear velocity",
            "Shows the speed of the object in world units per second.", true },

        { Flag::ObjectVelocityArrow, Group::Layers, Layer::Objects, Level::Full, "ObjectVelocityArrow", "Velocity arrow",
            "Draws an arrow pointing where the object is moving, which can differ from where it faces. The arrow has a fixed length set by the slider.", false },

        { Flag::ObjectAngularVelocity, Group::Layers, Layer::Objects, Level::Full, "ObjectAngularVelocity", "Angular velocity",
            "Shows the magnitude of the angular velocity as stored by the engine.", false },

        { Flag::ObjectDamage, Group::Layers, Layer::Objects, Level::Full, "ObjectDamage", "Damage received",
            "Shows the accumulated damage value of the object.", false },

        { Flag::ObjectParent, Group::Layers, Layer::Objects, Level::Full, "ObjectParent", "Parent handle",
            "Shows the handle of the parent object, when there is one.", false },

        { Flag::VehicleSeatSummary, Group::Layers, Layer::Objects, Level::Minimal, "VehicleSeatSummary", "Seat summary on vehicle label",
            "Adds one row per seat to the vehicle label with its state and occupant.", true },

        { Flag::VehicleSeatMarkers, Group::Layers, Layer::Objects, Level::Full, "VehicleSeatMarkers", "Seat markers",
            "Draws a label at the world position of each seat with state and distance to the local player.", false },

        { Flag::VehicleHijackerSlots, Group::Layers, Layer::Objects, Level::Full, "VehicleHijackerSlots", "Include hijacker slots",
            "Also lists the hijacker slots of each vehicle.", false },

        { Flag::HealthBars, Group::Layers, Layer::Health, Level::Minimal, "HealthBars", "Section health bars",
            "Draws a bar over each damage section sphere showing its remaining vitality.", true },

        { Flag::HealthSelectedOnly, Group::Layers, Layer::Health, Level::Minimal, "HealthSelectedOnly", "Selected object only",
            "Only the selected object gets health bars. Turn it off to draw bars for every object with health.", true, true },

        { Flag::HealthBarValue, Group::Layers, Layer::Health, Level::Full, "HealthBarValue", "Show value",
            "Writes the numeric vitality inside each bar.", false },

        { Flag::HealthBarTags, Group::Layers, Layer::Health, Level::Minimal, "HealthBarTags", "Show section tags",
            "Marks sections: K kills the object, H headshot, D destroys it, S shield.", true },

        { Flag::HealthShieldSections, Group::Layers, Layer::Health, Level::Minimal, "HealthShieldSections", "Include shield sections",
            "Also draws bars for shield sections.", true },

        { Flag::HealthIncludeCenter, Group::Layers, Layer::Health, Level::Full, "HealthIncludeCenter", "Include object-center anchors",
            "Also draws bars for sections that only have an imprecise object-center anchor.", false },

        { Flag::HealthTintSpheres, Group::Layers, Layer::Health, Level::Minimal, "HealthTintSpheres", "Tint aim spheres by vitality",
            "Colors each aim sphere from red to green according to its vitality, instead of its anchor source.", true },

        { Flag::AimSpheres, Group::Layers, Layer::Players, Level::Minimal, "AimSpheres", "Aim spheres",
            "Draws a sphere at each damage section of the objects.", true },

        { Flag::InteractionObject, Group::Layers, Layer::View, Level::Minimal, "InteractionObject", "Engine-highlighted object",
            "Marks the object the game engine currently offers to interact with, with the interaction kind and detail.", true },

        { Flag::InteractionMelee, Group::Layers, Layer::View, Level::Minimal, "InteractionMelee", "Melee target",
            "Marks the target the engine reports as available for melee.", true },

        { Flag::InteractionAim, Group::Layers, Layer::View, Level::Minimal, "InteractionAim", "Aim target",
            "Marks the object the local player is aiming at, with the target slot and model part.", true },

        { Flag::InteractionAimHitPoint, Group::Layers, Layer::View, Level::Minimal, "InteractionAimHitPoint", "Aim hit point",
            "Marks where the local player's camera ray lands, with its distance.", true },

        { Flag::InteractionLink, Group::Layers, Layer::View, Level::Minimal, "InteractionLink", "Arrow to target",
            "Draws an arrow from the local player to each marked interaction target.", true },

        { Flag::InteractionAffordanceDetails, Group::Layers, Layer::View, Level::Minimal, "InteractionAffordanceDetails", "Affordance details on target",
            "Adds the behaviors and activation of the affordance matching the engine-highlighted object.", true },

        { Flag::AffordanceAll, Group::Layers, Layer::View, Level::Full, "AffordanceAll", "Label every affordance",
            "Labels every object the local player can currently interact with, with role, behaviors, activation and distance.", false },

        { Flag::FixtureTeleportLabels, Group::Layers, Layer::Fixtures, Level::Minimal, "FixtureTeleportLabels", "Teleport labels",
            "Labels each teleporter with its kind (sender, receiver, two-way), channel, zone and what it lets through.", true },

        { Flag::FixtureTeleportLinks, Group::Layers, Layer::Fixtures, Level::Minimal, "FixtureTeleportLinks", "Teleport links",
            "Draws dashed arrows from each teleporter to its destinations.", true },

        { Flag::FixtureLiftLabels, Group::Layers, Layer::Fixtures, Level::Minimal, "FixtureLiftLabels", "Lift labels",
            "Labels each lift with its kind (vertical, curved, redirected) and tag.", true },

        { Flag::FixtureLiftArrows, Group::Layers, Layer::Fixtures, Level::Minimal, "FixtureLiftArrows", "Lift direction arrows",
            "Draws an arrow along the direction of the last stage of each lift.", true },

        { Flag::FixtureShieldLabels, Group::Layers, Layer::Fixtures, Level::Minimal, "FixtureShieldLabels", "Shield labels",
            "Labels each barrier with its kind (one-way, two-way, blocker, drop shield, portable) and tag.", true },

        { Flag::FixtureShieldArrows, Group::Layers, Layer::Fixtures, Level::Minimal, "FixtureShieldArrows", "Shield direction arrows",
            "Draws an arrow along the pass direction of one-way barriers.", true },

        { Flag::FixtureObjectives, Group::Layers, Layer::Fixtures, Level::Minimal, "FixtureObjectives", "Objectives",
            "Labels objectives (flags, bombs) and objective spawn zones with team and carrier.", true },

        { Flag::FixtureSpawns, Group::Layers, Layer::Fixtures, Level::Full, "FixtureSpawns", "Spawn points",
            "Labels player spawn points with kind and team.", false },

        { Flag::FixtureShieldTranslucent, Group::Layers, Layer::Fixtures, Level::Minimal, "FixtureShieldTranslucent", "Translucent shields",
            "Draws shield doors, walls and drop shields translucent so the geometry behind them stays visible.", true },

        { Flag::LimitKillVolumes, Group::Layers, Layer::World, Level::Minimal, "LimitKillVolumes", "Kill volumes",
            "Draws the trigger volumes that the scenario uses as kill triggers, translucent with an outline. Red kills at once and orange kills after a countdown.", true },

        { Flag::LimitSafeVolumes, Group::Layers, Layer::World, Level::Minimal, "LimitSafeVolumes", "Safe zone volumes",
            "Draws the trigger volumes that the scenario uses as safe zone triggers.", true },

        { Flag::LimitSoftCeilings, Group::Layers, Layer::World, Level::Minimal, "LimitSoftCeilings", "Soft ceilings",
            "Draws the soft ceiling surfaces of the structure design: blue pushes back, yellow kills softly and purple makes slip. Translucent, without outline.", true },

        { Flag::OverlayWireframe, Group::Layers, Layer::World, Level::Minimal, "OverlayWireframe", "Overlay: collision wireframe",
            "Also draws the map collision as a visible wireframe over the game frame, to check that it lines up with the walls of the game frame.", true },

        { Flag::HideGameHud, Group::Camera, Layer::Count, Level::Full, "HideGameHud", "Hide game HUD",
            "Skips the game's HUD passes. For clean captures.", false },

        { Flag::HideFirstPerson, Group::Camera, Layer::Count, Level::Full, "HideFirstPerson", "Hide first person",
            "Skips the game's first person arms and weapon. With the free camera they are always hidden and the game draws the player's body.", false },

        { Flag::WatchPov, Group::Camera, Layer::Count, Level::Full, "WatchPov", "Watch POV",
            "The game shows the selected player in first person, with its HUD (engine player mapping, D340).", false },

        { Flag::PossessBot, Group::Camera, Layer::Count, Level::Full, "PossessBot", "Auto-possess bot",
            "With the Game camera, the Artemis bot you select plays with your mouse and keyboard (your settings). Your own player stands still. TAB to the Free camera, selecting a human or turning this off gives the bot back. Pair it with Watch POV to see through the bot.", false },

        { Flag::DirectorAutoPlay, Group::Director, Layer::Count, Level::Full, "DirectorAutoPlay", "Auto-play on match start",
            "When a match starts the Viewer becomes active and the director plays the active sequence from the first frame it can, before the game shows the camera of the local player.", false },

        { Flag::AgentStrip, Group::Camera, Layer::Count, Level::Full, "AgentStrip", "Agent strip",
            "Draws the strip with the previous, current and next player at the top of the frame. Off: nothing is drawn, for clean frames.", true },

        { Flag::RosterAgentsOnly, Group::Camera, Layer::Count, Level::Full, "RosterAgentsOnly", "Roster: agents only",
            "The roster that the strip, the up and down arrows and the director's POV pick walk keeps only the Artemis bots. A selected human is then not in the roster.", false },

        { Flag::DrawOverlay, Group::Camera, Layer::Count, Level::Full, "DrawOverlay", "Draw debug overlay",
            "Master switch of the debug overlay: off, no debug 3D pass is drawn (map, objects, aim spheres, View, fixtures, limits) and no debug 3D labels. The Presentation layers (Agent observation) are not affected: they follow their own level. The camera, the director and the selection keep working. The card, the strip and the F3 HUD have their own switches.", true },
    } };

    inline constexpr std::array<ScalarInfo, k_ScalarCount> k_Scalars
    { {
        { Scalar::LabelMaxDistance, Group::Layers, Layer::Count, "LabelMaxDistance", "Max label distance",
            "Labels farther than this from the camera are hidden.", 150.0f, 5.0f, 1000.0f, "%.0f" },

        { Scalar::LabelMaxCount, Group::Layers, Layer::Count, "LabelMaxCount", "Max labels",
            "Upper bound of labels drawn per frame, nearest first.", 64.0f, 4.0f, 256.0f, "%.0f" },

        { Scalar::LabelTextScale, Group::Layers, Layer::Count, "LabelTextScale", "Text scale",
            "Multiplier applied to the label text size.", 1.0f, 0.5f, 2.0f, "%.2f" },

        { Scalar::LabelBackgroundAlpha, Group::Layers, Layer::Count, "LabelBackgroundAlpha", "Background opacity",
            "Opacity of the panel behind each label.", 0.45f, 0.0f, 1.0f, "%.2f" },

        { Scalar::ObjectArrowLength, Group::Layers, Layer::Objects, "ObjectArrowLength", "Velocity arrow length (wu)",
            "Length of the velocity arrow in world units. The arrow only shows the direction.", 1.5f, 0.25f, 10.0f, "%.2f" },

        { Scalar::HealthBarWidth, Group::Layers, Layer::Health, "HealthBarWidth", "Bar width (px)",
            "Width of each section health bar before text scaling.", 46.0f, 20.0f, 120.0f, "%.0f" },

        { Scalar::FixtureArrowLength, Group::Layers, Layer::Fixtures, "FixtureArrowLength", "Fixture arrow length (wu)",
            "Length of lift and shield direction arrows in world units.", 3.0f, 0.5f, 15.0f, "%.1f" },

        { Scalar::ShieldOpacity, Group::Layers, Layer::Fixtures, "ShieldOpacity", "Shield opacity",
            "Opacity of translucent shields. Lower values are more see-through.", 0.30f, 0.05f, 1.0f, "%.2f" },

        { Scalar::LimitOpacity, Group::Layers, Layer::World, "LimitOpacity", "Volume opacity",
            "Opacity of the kill and safe zone volumes and the soft ceilings. Lower values are more see-through.", 0.20f, 0.02f, 1.0f, "%.2f" },

        { Scalar::SpeedCustom, Group::Camera, Layer::Count, "SpeedCustom", "Custom speed (wu/s)",
            "Free camera speed while Ctrl is held. Ctrl + mouse wheel changes it.", 3.0f, 0.1f, 60.0f, "%.2f" },

        { Scalar::FovNormal, Group::Camera, Layer::Count, "FovNormal", "Field of view (deg)",
            "Vertical field of view of the free camera. The mouse wheel changes it.", 60.0f, 30.0f, 110.0f, "%.0f" },

        { Scalar::FovZoom, Group::Camera, Layer::Count, "FovZoom", "Zoom field of view (deg)",
            "Field of view of the free camera while the zoom is active. The mouse wheel changes it.", 25.0f, 5.0f, 60.0f, "%.0f" },

        { Scalar::SceneColorScale, Group::Development, Layer::Count, "SceneColorScale", "Scene color scale",
            "Multiplies the overlay colors, from 2^-12 to 2^4 (logarithmic). The default 2^-5 matches the HDR target of the game (D335). Double click to go back to the default.", 0.03125f, 0.000244140625f, 16.0f, "%.4g", true },

        { Scalar::DirectorDegreesPerSecond, Group::Director, Layer::Count, "DirectorDegreesPerSecond", "Orbit speed (deg/s)",
            "Turn speed of the orbit shots added from now on. 6 is one turn per minute, negative turns the other way.", 6.0f, -60.0f, 60.0f, "%.1f" },
    } };

    // return: Whether the flag is on when its layer is at the given level. A flag without a layer is not derived from a level: false.
    constexpr auto IsFlagOn(const FlagInfo& info, Level level) -> bool
    {
        if (info.Owner == Layer::Count) return false;
        if (info.IsInverted) return level == Level::Minimal;
        return level >= info.MinLevel;
    }

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

        for (std::size_t i = 0; i < k_LayerCount; ++i)
        {
            if (static_cast<std::size_t>(k_Layers[i].Id) != i) return false;
        }

        return true;
    }

    // return: Whether the layer is a Presentation one, which the master switch of the debug overlay does not turn off.
    constexpr auto IsPresentationLayer(Layer layer) -> bool
    {
        return k_Layers[static_cast<std::size_t>(layer)].Kind == LayerKind::Presentation;
    }

    // Which 3D passes of the scene point run in a frame, and what they need.
    struct ScenePasses
    {
        // The debug overlay: every pass but the observation one, and its labels.
        bool DrawDebug{};

        // The arrows of the observation layer (a Presentation 3D layer).
        bool DrawObs{};

        // The map collision is drawn in depth only (invisible) so the walls hide the arrows. The debug overlay draws the map itself.
        bool NeedMapDepth{};

        // The game must keep the depth copies of the first person mask, so the arms and the weapon stay over what is drawn.
        bool NeedMask{};
    };

    // return: The passes of a frame from the master switch of the debug overlay and the level of the observation layer. With the master off and the
    // observation layer off nothing runs (no pass, no GPU state touched, no mask).
    constexpr auto DecideScenePasses(bool isMasterOn, Level obsLevel) -> ScenePasses
    {
        const bool isObsOn{ obsLevel != Level::Off };

        return ScenePasses{ .DrawDebug = isMasterOn, .DrawObs = isObsOn, .NeedMapDepth = isObsOn && !isMasterOn, .NeedMask = isMasterOn || isObsOn };
    }

    // return: True if, with every layer at its default level (Minimal, Off for Obs), each flag of a layer gives the value it had before the layers.
    consteval auto DoLayersKeepTheOldDefaults() -> bool
    {
        for (const FlagInfo& info : k_Flags)
        {
            if (info.Owner == Layer::Count) continue;
            if (info.MinLevel == Level::Off) return false;
            if (IsFlagOn(info, k_Layers[static_cast<std::size_t>(info.Owner)].Default) != info.Default) return false;
        }

        // note: Only a Presentation layer may start off (VW.17, the observation layer): it changes the look of the frame for whoever has not asked for it.
        for (const LayerInfo& info : k_Layers)
        {
            if (info.Default != Level::Minimal && !(IsPresentationLayer(info.Id) && info.Default == Level::Off)) return false;
        }

        return true;
    }

    static_assert(AreTablesOrdered(), "Viewer option tables must follow enum order.");
    static_assert(DoLayersKeepTheOldDefaults(), "Every layer at Minimal must give the defaults the flags had before the layers.");
}