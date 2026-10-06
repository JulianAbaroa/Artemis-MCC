export module Common.Tag.Type;

import std;

export namespace Common::Tag::Type
{
    // Tag names of the playable bipeds.
    namespace Biped
    {
        constexpr const char* k_Spartan{ "objects\\characters\\spartans\\spartans" };
        constexpr const char* k_Elite{ "objects\\characters\\elite\\elite" };

        constexpr auto IsBiped(const std::string& tag) -> bool
        {
            return tag == k_Spartan || tag == k_Elite;
        }

        constexpr auto IsSpartan(const std::string& tag) -> bool
        {
            return tag == k_Spartan;
        }

        constexpr auto IsElite(const std::string& tag) -> bool
        {
            return tag == k_Elite;
        }
    }

    // Tag names of the armor abilities.
    namespace ArmorAbility
    {
        constexpr const char* k_Sprint{ "objects\\equipment\\sprint\\sprint" };
        constexpr const char* k_Jetpack{ "objects\\equipment\\jet_pack\\jet_pack" };
        constexpr const char* k_Hologram{ "objects\\equipment\\hologram\\hologram" };
        constexpr const char* k_Evade{ "objects\\equipment\\evade\\evade" };
        constexpr const char* k_DropShield{ "objects\\equipment\\drop_shield\\drop_shield" };
        constexpr const char* k_ArmorLockup{ "objects\\equipment\\armor_lockup\\armor_lockup" };
        constexpr const char* k_ActiveCamouflage{ "objects\\equipment\\active_camouflage\\active_camouflage" };

        constexpr auto IsArmorAbility(const std::string& tag) -> bool
        {
            return tag == k_Sprint || tag == k_Jetpack || tag == k_Hologram ||
                tag == k_Evade || tag == k_DropShield || tag == k_ArmorLockup ||
                tag == k_ActiveCamouflage;
        }
    }

    // Tag names of the ammo pickups.
    namespace Ammo
    {
        constexpr const char* k_AmmoCabinet{ "objects\\gear\\human\\military\\ammo_box\\ammo_box" };
        constexpr const char* k_RocketAmmo{ "objects\\gear\\human\\military\\rocket_launcher_ammo\\rocket_launcher_ammo" };
        constexpr const char* k_SniperAmmo{ "objects\\gear\\human\\military\\sniper_rifle_ammo\\sniper_rifle_ammo" };

        constexpr auto IsAmmo(const std::string& tag) -> bool
        {
            return tag == k_AmmoCabinet || tag == k_RocketAmmo || tag == k_SniperAmmo;
        }
    }

    // Tag names of the powerups.
    namespace Powerup
    {
        constexpr const char* k_BluePowerup{ "objects\\multi\\powerups\\powerup_blue\\powerup_blue" };
        constexpr const char* k_RedPowerup{ "objects\\multi\\powerups\\powerup_red\\powerup_red" };
        constexpr const char* k_YellowPowerup{ "objects\\multi\\powerups\\powerup_yellow\\powerup_yellow" };

        constexpr auto IsPowerup(const std::string& tag) -> bool
        {
            return tag == k_BluePowerup || tag == k_RedPowerup || tag == k_YellowPowerup;
        }
    }

    // Tag names of the spawn points.
    namespace Spawn
    {
        constexpr const char* k_InvisibleRespawnPoint{ "objects\\multi\\spawning\\respawn_point_invisible" };
        constexpr const char* k_InitialSpawnPoint{ "objects\\multi\\spawning\\initial_spawn_point" };
        constexpr const char* k_RespawnPoint{ "objects\\multi\\spawning\\respawn_point" };

        constexpr auto IsSpawn(const std::string& tag) -> bool
        {
            return tag == k_InvisibleRespawnPoint || tag == k_InitialSpawnPoint ||
                tag == k_RespawnPoint;
        }

        constexpr auto IsInitialSpawn(const std::string& tag) -> bool
        {
            return tag == k_InitialSpawnPoint;
        }

        constexpr auto IsRespawn(const std::string& tag) -> bool
        {
            return tag == k_RespawnPoint;
        }

        constexpr auto IsInvisibleSpawn(const std::string& tag) -> bool
        {
            return tag == k_InvisibleRespawnPoint;
        }
    }

    // Tag names of the boundary volumes.
    namespace Boundary
    {
        constexpr const char* k_SafeBoundary{ "objects\\multi\\boundaries\\safe_volume" };
        constexpr const char* k_SoftSafeBoundary{ "objects\\multi\\boundaries\\soft_safe_volume" };
        constexpr const char* k_KillBoundary{ "objects\\multi\\boundaries\\kill_volume" };
        constexpr const char* k_SoftKillBoundary{ "objects\\multi\\boundaries\\soft_kill_volume" };

        constexpr auto IsBoundary(const std::string& tag) -> bool
        {
            return tag == k_SafeBoundary || tag == k_SoftSafeBoundary ||
                tag == k_KillBoundary || tag == k_SoftKillBoundary;
        }
    }

    // Tag names of the explosive props.
    namespace Explosive
    {
        constexpr const char* k_FusionCoil{ "objects\\gear\\human\\military\\fusion_coil\\fusion_coil" };
        constexpr const char* k_Landmine{ "objects\\multi\\land_mine\\land_mine" };
        constexpr const char* k_PlasmaBattery{ "objects\\props\\covenant\\battery\\battery" };
        constexpr const char* k_PropaneTank{ "objects\\gear\\human\\industrial\\propane_tank\\propane_tank" };

        constexpr auto IsExplosive(const std::string& tag) -> bool
        {
            return tag == k_FusionCoil || tag == k_Landmine ||
                tag == k_PlasmaBattery || tag == k_PropaneTank;
        }
    }

    // Tag names of the man cannons and gravity lifts.
    namespace Lift
    {
        constexpr const char* k_ManCannon{ "objects\\levels\\forge\\ff_man_cannon_forge\\ff_man_cannon_forge" };
        constexpr const char* k_ManCannonHeavy{ "objects\\levels\\forge\\ff_man_cannon_forge_heavy\\ff_man_cannon_forge_heavy" };
        constexpr const char* k_ManCannonLight{ "objects\\levels\\forge\\ff_man_cannon_forge_light\\ff_man_cannon_forge_light" };
        constexpr const char* k_VehicleManCannon{ "objects\\levels\\forge\\ff_veh_man_cannon\\ff_veh_man_cannon" };
        constexpr const char* k_GravityLift{ "objects\\levels\\forge\\ff_grav_lift\\ff_grav_lift" };
        constexpr const char* k_ManCannonMCC{ "objects\\multi\\dlc\\dlc_medium_mancannon\\dlc_medium_mancannon" };
        constexpr const char* k_ManCannonHeavyMCC{ "objects\\multi\\dlc\\dlc_medium_mancannon_heavy\\dlc_medium_mancannon_heavy" };
        constexpr const char* k_ManCannonLightMCC{ "objects\\multi\\dlc\\dlc_medium_mancannon_light\\dlc_medium_mancannon_light" };
        constexpr const char* k_ForerunnerGravityLift{ "objects\\cex\\cex_hangemhigh\\crates\\heh_gravlift_tunnels\\heh_gravlift_tunnels" };
        constexpr const char* k_ForerunnerGravityLiftTall{ "objects\\cex\\cex_hangemhigh\\crates\\heh_gravlift\\heh_gravlift" };
        constexpr const char* k_ManCannonHumanMCC{ "objects\\cex\\cex_headlong\\crates\\man_cannon\\man_cannon" };

        constexpr auto IsLift(const std::string& tag) -> bool
        {
            return tag == k_ManCannon || tag == k_ManCannonHeavy ||
                tag == k_ManCannonLight || tag == k_VehicleManCannon ||
                tag == k_GravityLift || tag == k_ManCannonMCC ||
                tag == k_ManCannonHeavyMCC || tag == k_ManCannonLightMCC ||
                tag == k_ForerunnerGravityLift || tag == k_ForerunnerGravityLiftTall ||
                tag == k_ManCannonHumanMCC;
        }

        constexpr auto IsDefault(const std::string& tag) -> bool
        {
            return tag == k_ManCannon || tag == k_ManCannonMCC ||
                tag == k_GravityLift || tag == k_ForerunnerGravityLift;
        }

        constexpr auto IsHeavy(const std::string& tag) -> bool
        {
            return tag == k_ManCannonHeavy || tag == k_ManCannonHeavyMCC ||
                tag == k_ManCannonHumanMCC || tag == k_ForerunnerGravityLiftTall;
        }

        constexpr auto IsLight(const std::string& tag) -> bool
        {
            return tag == k_ManCannonLight || tag == k_ManCannonLightMCC;
        }

        constexpr auto IsVehicle(const std::string& tag) -> bool
        {
            return tag == k_VehicleManCannon;
        }

        constexpr auto IsCurved(const std::string& tag) -> bool
        {
            return tag == k_ManCannon || tag == k_ManCannonHeavy ||
                tag == k_ManCannonLight || tag == k_ManCannonMCC ||
                tag == k_ManCannonHeavyMCC || tag == k_ManCannonLightMCC ||
                tag == k_ManCannonHumanMCC;
        }

        constexpr auto IsVertical(const std::string& tag) -> bool
        {
            return tag == k_GravityLift || tag == k_VehicleManCannon;
        }

        constexpr auto IsRedirected(const std::string& tag) -> bool
        {
            return tag == k_ForerunnerGravityLift ||
                tag == k_ForerunnerGravityLiftTall;
        }
    }

    // Tag names of the objective objects of the game types.
    namespace Objective
    {
        // CTF.
        constexpr const char* k_FlagStand{ "objects\\multi\\models\\mp_flag_base\\mp_flag_base" };
        constexpr const char* k_Flag{ "objects\\weapons\\multiplayer\\flag\\flag" };

        // Assault.
        constexpr const char* k_CapturePlate{ "objects\\multi\\models\\mp_circle\\mp_circle" };
        constexpr const char* k_Bomb{ "objects\\weapons\\multiplayer\\assault_bomb\\assault_bomb" };

        // Zone.
        constexpr const char* k_HillMarker{ "objects\\multi\\models\\mp_hill_beacon\\mp_hill_beacon" };

        constexpr auto IsObjectiveSpawn(const std::string& tag) -> bool
        {
            return tag == k_FlagStand || tag == k_CapturePlate;
        }

        constexpr auto IsObjectiveZone(const std::string& tag) -> bool
        {
            return tag == k_HillMarker;
        }

        constexpr auto IsObjective(const std::string& tag) -> bool
        {
            return tag == k_Flag || tag == k_Bomb;
        }
    }

    // Tag names of the shield doors and walls.
    namespace Shield
    {
        constexpr const char* k_OneWayDoorSmall{ "objects\\levels\\forge\\ff_one_way_door_small\\ff_one_way_door_small" };
        constexpr const char* k_OneWayDoorMedium{ "objects\\levels\\forge\\ff_one_way_door_medium\\ff_one_way_door_medium" };
        constexpr const char* k_OneWayDoorLarge{ "objects\\levels\\forge\\ff_one_way_door_large\\ff_one_way_door_large" };
        constexpr const char* k_TwoWayDoorSmall{ "objects\\levels\\forge\\ff_two_way_door_small\\ff_two_way_door_small" };
        constexpr const char* k_TwoWayDoorMedium{ "objects\\levels\\forge\\ff_two_way_door_medium\\ff_two_way_door_medium" };
        constexpr const char* k_TwoWayDoorLarge{ "objects\\levels\\forge\\ff_two_way_door_large\\ff_two_way_door_large" };
        constexpr const char* k_OneWayDoorXSmall{ "objects\\levels\\forge\\ff_one_way_door_xsmall\\ff_one_way_door_xsmall" };
        constexpr const char* k_OneWayDoorGarage{ "objects\\levels\\forge\\ff_one_way_door_garage\\ff_one_way_door_garage" };
        constexpr const char* k_ShieldWallSmall{ "objects\\levels\\multi\\70_boneyard\\frigate_laser_field_small\\frigate_laser_field_small" };
        constexpr const char* k_ShieldWallMedium{ "objects\\levels\\multi\\70_boneyard\\frigate_laser_field_med\\frigate_laser_field_med" };
        constexpr const char* k_ShieldWallLarge{ "objects\\levels\\multi\\70_boneyard\\frigate_laser_field_wall\\frigate_laser_field_wall" };
        constexpr const char* k_ShieldWallXLarge{ "objects\\levels\\multi\\70_boneyard\\frigate_laser_field_gate\\frigate_laser_field_gate" };
        constexpr const char* k_ShieldDoorSmall{ "objects\\levels\\forge\\ff_shield_door_small\\ff_shield_door_small" };
        constexpr const char* k_ShieldDoorMedium{ "objects\\levels\\forge\\ff_shield_door_medium\\ff_shield_door_medium" };
        constexpr const char* k_ShieldDoorLarge{ "objects\\levels\\forge\\ff_shield_door_large\\ff_shield_door_large" };
        constexpr const char* k_HangarShieldDoorSmall{ "levels\\multi\\dlc\\objects\\dlc_slayer_shield_repair_s\\dlc_hangar_shield_repair_s\\dlc_hangar_shield_repair_s" };
        constexpr const char* k_HangarShieldDoorSmallSolid{ "levels\\multi\\dlc\\objects\\dlc_hangar_forcefield_small_solid\\dlc_hangar_forcefield_small_solid" };
        constexpr const char* k_HangarShieldDoorLarge{ "objects\\levels\\multi\\dlc\\dlc_slayer\\dlc_hangar_shield\\dlc_hangar_shield" };
        constexpr const char* k_HangarShieldDoorLargeSolid{ "levels\\multi\\dlc\\objects\\dlc_hangar_forcefield_solid\\dlc_hangar_forcefield_solid" };

        constexpr const char* k_PortableShield{ "objects\\props\\covenant\\cov_portable_shield\\cov_portable_shield" };

        constexpr const char* k_DropShield{ "objects\\equipment\\drop_shield\\drop_shield_shield\\drop_shield_shield" };

        constexpr auto IsShield(const std::string& tag) -> bool
        {
            return tag == k_OneWayDoorSmall || tag == k_OneWayDoorMedium ||
                tag == k_OneWayDoorLarge || tag == k_TwoWayDoorSmall ||
                tag == k_TwoWayDoorMedium || tag == k_TwoWayDoorLarge ||
                tag == k_OneWayDoorXSmall || tag == k_OneWayDoorGarage ||
                tag == k_ShieldWallSmall || tag == k_ShieldWallMedium ||
                tag == k_ShieldWallLarge || tag == k_ShieldWallXLarge ||
                tag == k_ShieldDoorSmall || tag == k_ShieldDoorMedium ||
                tag == k_ShieldDoorLarge || tag == k_HangarShieldDoorSmall ||
                tag == k_HangarShieldDoorSmallSolid || tag == k_HangarShieldDoorLarge ||
                tag == k_HangarShieldDoorLargeSolid || tag == k_DropShield;
        }

        constexpr auto IsOneWay(const std::string& tag) -> bool
        {
            return tag == k_OneWayDoorSmall || tag == k_OneWayDoorMedium ||
                tag == k_OneWayDoorLarge || tag == k_OneWayDoorXSmall ||
                tag == k_OneWayDoorGarage || tag == k_ShieldDoorSmall ||
                tag == k_ShieldDoorMedium || tag == k_ShieldDoorLarge;
        }

        constexpr auto IsTwoWay(const std::string& tag) -> bool
        {
            return tag == k_TwoWayDoorSmall || tag == k_TwoWayDoorMedium ||
                tag == k_TwoWayDoorLarge || tag == k_HangarShieldDoorSmall ||
                tag == k_HangarShieldDoorLarge || tag == k_DropShield;
        }

        constexpr auto IsBlocker(const std::string& tag) -> bool
        {
            return tag == k_ShieldWallSmall || tag == k_ShieldWallMedium ||
                tag == k_ShieldWallLarge || tag == k_ShieldWallXLarge ||
                tag == k_HangarShieldDoorSmallSolid ||
                tag == k_HangarShieldDoorLargeSolid;
        }

        constexpr auto IsShieldDoor(const std::string& tag) -> bool
        {
            return tag == k_ShieldDoorSmall || tag == k_ShieldDoorMedium ||
                tag == k_ShieldDoorLarge;
        }

        constexpr auto IsPortableShield(const std::string& tag) -> bool
        {
            return tag == k_PortableShield;
        }
    }

    // Tag names of the teleporters.
    namespace Teleport
    {
        constexpr const char* k_TeleportSender{ "objects\\levels\\forge\\ff_teleporter_sender\\ff_teleporter_sender" };
        constexpr const char* k_TeleportReceiver{ "objects\\levels\\forge\\ff_teleporter_receiver\\ff_teleporter_receiver" };
        constexpr const char* k_TeleportTwoWay{ "objects\\levels\\forge\\ff_teleporter_2_way\\ff_teleporter_2_way" };

        constexpr auto IsTeleport(const std::string& tag) -> bool
        {
            return tag == k_TeleportSender || tag == k_TeleportReceiver ||
                tag == k_TeleportTwoWay;
        }

        constexpr auto IsReceiver(const std::string& tag) -> bool
        {
            return tag == k_TeleportReceiver;
        }

        constexpr auto IsSender(const std::string& tag) -> bool
        {
            return tag == k_TeleportSender;
        }

        constexpr auto IsTwoWay(const std::string& tag) -> bool
        {
            return tag == k_TeleportTwoWay;
        }
    }

    // Tag names of the weapons, grenades and weapon turrets.
    namespace Weapon
    {
        constexpr const char* k_AssaultRifle{ "objects\\weapons\\rifle\\assault_rifle\\assault_rifle" };
        constexpr const char* k_DMR{ "objects\\weapons\\rifle\\dmr\\dmr" };
        constexpr const char* k_GrenadeLauncher{ "objects\\weapons\\rifle\\grenade_launcher\\grenade_launcher" };
        constexpr const char* k_Magnum{ "objects\\weapons\\pistol\\magnum\\magnum" };
        constexpr const char* k_RocketLauncher{ "objects\\weapons\\support_high\\rocket_launcher\\rocket_launcher" };
        constexpr const char* k_Shotgun{ "objects\\weapons\\rifle\\shotgun\\shotgun" };
        constexpr const char* k_SniperRifle{ "objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle" };
        constexpr const char* k_SpartanLaser{ "objects\\weapons\\support_high\\spartan_laser\\spartan_laser" };
        constexpr const char* k_FragGrenade{ "objects\\weapons\\grenade\\frag_grenade\\frag_grenade" };
        constexpr const char* k_VehicleMountedMachinegun{ "objects\\vehicles\\human\\turrets\\machinegun\\machinegun" };
        constexpr const char* k_MountedMachinegun{ "objects\\vehicles\\human\\turrets\\machinegun\\weapon\\machinegun_turret_mounted\\machinegun_mounted" };
        constexpr const char* k_Machinegun{ "objects\\vehicles\\human\\turrets\\machinegun\\weapon\\machinegun_turret\\machinegun_turret" };
        constexpr const char* k_ConcussionRifle{ "objects\\weapons\\rifle\\concussion_rifle\\concussion_rifle" };
        constexpr const char* k_EnergySword{ "objects\\weapons\\melee\\energy_sword\\energy_sword" };
        constexpr const char* k_FuelRod{ "objects\\weapons\\support_high\\flak_cannon\\flak_cannon" };
        constexpr const char* k_GravityHammer{ "objects\\weapons\\melee\\gravity_hammer\\gravity_hammer" };
        constexpr const char* k_FocusRifle{ "objects\\weapons\\rifle\\focus_rifle\\focus_rifle" };
        constexpr const char* k_NeedleRifle{ "objects\\weapons\\rifle\\needle_rifle\\needle_rifle" };
        constexpr const char* k_Needler{ "objects\\weapons\\pistol\\needler\\needler" };
        constexpr const char* k_PlasmaLauncher{ "objects\\weapons\\support_high\\plasma_launcher\\plasma_launcher" };
        constexpr const char* k_PlasmaPistol{ "objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol" };
        constexpr const char* k_PlasmaRepeater{ "objects\\weapons\\rifle\\plasma_repeater\\plasma_repeater" };
        constexpr const char* k_PlasmaRifle{ "objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle" };
        constexpr const char* k_Spiker{ "objects\\weapons\\rifle\\spike_rifle\\spike_rifle" };
        constexpr const char* k_PlasmaGrenade{ "objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade" };
        constexpr const char* k_VehicleMountedPlasmaTurret{ "objects\\vehicles\\covenant\\turrets\\plasma_turret\\plasma_turret_mounted" };
        constexpr const char* k_MountedPlasmaTurret{ "objects\\vehicles\\covenant\\turrets\\plasma_turret\\weapon\\plasma_turret_mounted\\plasma_turret_mounted" };
        constexpr const char* k_PlasmaTurret{ "objects\\vehicles\\covenant\\turrets\\plasma_turret\\weapon\\plasma_turret\\plasma_turret" };

        constexpr auto IsWeapon(const std::string& tag) -> bool
        {
            return tag == k_AssaultRifle || tag == k_DMR ||
                tag == k_GrenadeLauncher || tag == k_Magnum ||
                tag == k_RocketLauncher || tag == k_Shotgun ||
                tag == k_SniperRifle || tag == k_SpartanLaser ||
                tag == k_MountedMachinegun || tag == k_Machinegun ||
                tag == k_ConcussionRifle || tag == k_EnergySword ||
                tag == k_FuelRod || tag == k_GravityHammer ||
                tag == k_FocusRifle || tag == k_NeedleRifle ||
                tag == k_Needler || tag == k_PlasmaLauncher ||
                tag == k_PlasmaPistol || tag == k_PlasmaRepeater ||
                tag == k_PlasmaRifle || tag == k_Spiker ||
                tag == k_MountedPlasmaTurret || tag == k_PlasmaTurret;
        }

        constexpr auto IsGrenade(const std::string& tag) -> bool
        {
            return tag == k_FragGrenade || tag == k_PlasmaGrenade;
        }

        constexpr auto IsVehicle(const std::string& tag) -> bool
        {
            return tag == k_VehicleMountedPlasmaTurret ||
                tag == k_MountedMachinegun;
        }
    }

    // Tag names of the vehicles and their turrets.
    namespace Vehicle
    {
        constexpr const char* k_Banshee{ "objects\\vehicles\\covenant\\banshee\\banshee" };
        constexpr const char* k_Falcon{ "objects\\vehicles\\human\\falcon\\falcon" };
        constexpr const char* k_FalconSensor{ "objects\\vehicles\\human\\falcon\\turrets\\falcon_sensor\\falcon_sensor" };
        constexpr const char* k_FalconTurretRight{ "objects\\vehicles\\human\\falcon\\turrets\\falcon_side_gun_right\\falcon_side_gun_right" };
        constexpr const char* k_FalconTurretLeft{ "objects\\vehicles\\human\\falcon\\turrets\\falcon_side_gun_left\\falcon_side_gun_left" };
        constexpr const char* k_FalconGrenadeRight{ "objects\\vehicles\\human\\falcon\\turrets\\falcon_side_grenade_right\\falcon_side_grenade_right" };
        constexpr const char* k_FalconGrenadeLeft{ "objects\\vehicles\\human\\falcon\\turrets\\falcon_side_grenade_left\\falcon_side_grenade_left" };
        constexpr const char* k_FalconChinGun{ "objects\\vehicles\\human\\falcon\\turrets\\falcon_chin_gun\\falcon_chin_gun" };
        constexpr const char* k_Ghost{ "objects\\vehicles\\covenant\\ghost\\ghost" };
        constexpr const char* k_Mongoose{ "objects\\vehicles\\human\\mongoose\\mongoose" };
        constexpr const char* k_Revenant{ "objects\\vehicles\\covenant\\revenant\\revenant" };
        constexpr const char* k_RevenantPlasmaTurret{ "objects\\vehicles\\covenant\\revenant\\turrets\\revenant_plasma_turret\\revenant_plasma_turret" };
        constexpr const char* k_Scorpion{ "objects\\vehicles\\human\\scorpion\\scorpion" };
        constexpr const char* k_ScorpionTurret{ "objects\\vehicles\\human\\scorpion\\turrets\\scorpion_anti_infantry\\scorpion_anti_infantry" };
        constexpr const char* k_ScorpionCannon{ "objects\\vehicles\\human\\scorpion\\turrets\\scorpion_cannon\\scorpion_cannon" };
        constexpr const char* k_Shade{ "objects\\vehicles\\covenant\\turrets\\shade\\shade" };
        constexpr const char* k_ShadePlasmaCannon{ "objects\\vehicles\\covenant\\turrets\\shade\\weapons\\shade_plasma_cannon\\shade_plasma_cannon" };
        constexpr const char* k_ShadeFlakCannon{ "objects\\vehicles\\covenant\\turrets\\shade\\weapons\\shade_flak_cannon\\shade_flak_cannon" };
        constexpr const char* k_Warthog{ "objects\\vehicles\\human\\warthog\\warthog" };
        constexpr const char* k_WarthogChaingun{ "objects\\vehicles\\human\\warthog\\weapons\\warthog_chaingun\\warthog_chaingun" };
        constexpr const char* k_WarthogGauss{ "objects\\vehicles\\human\\warthog\\weapons\\warthog_gauss\\warthog_gauss" };
        constexpr const char* k_WarthogRocket{ "objects\\vehicles\\human\\warthog\\weapons\\warthog_rocket\\warthog_rocket" };
        constexpr const char* k_WarthogTroop{ "objects\\vehicles\\human\\warthog\\weapons\\warthog_troop\\warthog_troop" };
        constexpr const char* k_Wraith{ "objects\\vehicles\\covenant\\wraith\\wraith" };
        constexpr const char* k_WraithPlasmaTurret{ "objects\\vehicles\\covenant\\wraith\\turrets\\wraith_anti_infantry\\wraith_anti_infantry" };
        constexpr const char* k_WraithCannon{ "objects\\vehicles\\covenant\\wraith\\turrets\\wraith_mortar\\wraith_mortar" };
        constexpr const char* k_Sabre{ "objects\\vehicles\\human\\sabre\\sabre" };
        constexpr const char* k_Seraph{ "objects\\vehicles\\covenant\\seraph\\seraph" };
        constexpr const char* k_CartElectric{ "objects\\vehicles\\human\\civilian\\cart_electric\\cart_electric" };
        constexpr const char* k_Forklift{ "objects\\vehicles\\human\\civilian\\forklift\\forklift" };
        constexpr const char* k_Pickup{ "objects\\vehicles\\human\\civilian\\pickup\\pickup" };
        constexpr const char* k_TruckCab{ "objects\\vehicles\\human\\civilian\\truck_cab_large\\truck_cab_large" };
        constexpr const char* k_TruckCabBedLong{ "objects\\vehicles\\human\\civilian\\truck_cab_large\\attachments\\bed_long\\bed_long" };
        constexpr const char* k_OniVan{ "objects\\vehicles\\human\\civilian\\oni_van\\oni_van" };

        constexpr auto IsVehicle(const std::string& tag) -> bool
        {
            return
                tag == k_Banshee ||
                tag == k_Falcon || tag == k_FalconSensor ||
                tag == k_FalconTurretRight || tag == k_FalconTurretLeft ||
                tag == k_FalconGrenadeRight || tag == k_FalconGrenadeLeft ||
                tag == k_FalconChinGun ||
                tag == k_Ghost ||
                tag == k_Mongoose ||
                tag == k_Revenant ||
                tag == k_RevenantPlasmaTurret ||
                tag == k_Scorpion || tag == k_ScorpionTurret ||
                tag == k_ScorpionCannon ||
                tag == k_Shade || tag == k_ShadePlasmaCannon ||
                tag == k_ShadeFlakCannon ||
                tag == k_Warthog || tag == k_WarthogChaingun ||
                tag == k_WarthogGauss || tag == k_WarthogRocket ||
                tag == k_WarthogTroop ||
                tag == k_Wraith || tag == k_WraithPlasmaTurret ||
                tag == k_WraithCannon ||
                tag == k_Sabre ||
                tag == k_Seraph ||
                tag == k_CartElectric ||
                tag == k_Forklift ||
                tag == k_Pickup ||
                tag == k_TruckCab || tag == k_TruckCabBedLong ||
                tag == k_OniVan;
        }

        constexpr auto HasBoost(const std::string& tag) -> bool
        {
            return tag == k_Banshee || tag == k_Ghost ||
                tag == k_Wraith || tag == k_Revenant;
        }
    }

    // Tag names of the pallets.
    namespace Pallets
    {
        constexpr const char* k_Pallet{ "objects\\gear\\human\\industrial\\pallet\\pallet" };
        constexpr const char* k_PalletLarge{ "objects\\gear\\human\\industrial\\pallet_large\\pallet_large" };

        constexpr auto IsPallet(const std::string& tag) -> bool
        {
            return tag == k_Pallet || tag == k_PalletLarge;
        }
    }

    // Tag names of the control devices.
    namespace ControlDevice
    {
        constexpr const char* k_HealthStation{ "objects\\levels\\shared\\device_controls\\health_station\\health_station" };

        constexpr auto IsHealthStation(const std::string& tag) -> bool
        {
            return tag == k_HealthStation;
        }
    }

    // Tag names of the sky boxes.
    namespace SkyBox
    {
        constexpr const char* k_ForgeWorld{ "levels\\multi\\forge_halo\\sky_halo\\sky_halo" };
        constexpr const char* k_WinterContingency{ "levels\\solo\\m10\\sky_morning_overcast\\sky_morning_overcast" };
        constexpr const char* k_OniSwordBase{ "levels\\solo\\m20\\sky_morning\\sky_morning" };
        constexpr const char* k_Nightfall{ "levels\\solo\\m30\\sky_night\\sky_night" };
        constexpr const char* k_TipOfTheSpear{ "levels\\solo\\m35\\sky_daytime\\sky_daytime" };

        constexpr auto IsSkyBox(const std::string& tag) -> bool
        {
            return tag == k_ForgeWorld || tag == k_WinterContingency ||
                tag == k_OniSwordBase || tag == k_Nightfall ||
                tag == k_TipOfTheSpear;
        }
    }
}