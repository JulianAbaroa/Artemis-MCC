#pragma once

#include <string>

namespace TagName
{
	namespace Biped
	{
		constexpr const char* Spartan = "objects\\characters\\spartans\\spartans";
		constexpr const char* Elite = "objects\\characters\\elite\\elite";

		inline bool IsBiped(const std::string& tag)
		{
			return tag == Spartan || tag == Elite;
		}

		inline bool IsSpartan(const std::string& tag)
		{
			return tag == Spartan;
		}

		inline bool IsElite(const std::string& tag)
		{
			return tag == Elite;
		}
	}

	namespace ArmorAbility
	{
		constexpr const char* Sprint = "objects\\equipment\\sprint\\sprint";
		constexpr const char* Jetpack = "objects\\equipment\\jet_pack\\jet_pack";
		constexpr const char* Hologram = "objects\\equipment\\hologram\\hologram";
		constexpr const char* Evade = "objects\\equipment\\evade\\evade";
		constexpr const char* DropShield = "objects\\equipment\\drop_shield\\drop_shield";
		constexpr const char* ArmorLockup = "objects\\equipment\\armor_lockup\\armor_lockup";
		constexpr const char* ActiveCamouflage = "objects\\equipment\\active_camouflage\\active_camouflage";

		inline bool IsArmorAbility(const std::string& tag)
		{
			return tag == Sprint || tag == Jetpack || tag == Hologram ||
				tag == Evade || tag == DropShield || tag == ArmorLockup ||
				tag == ActiveCamouflage;
		}
	}

	namespace Ammo
	{
		constexpr const char* AmmoCabinet = "objects\\gear\\human\\military\\ammo_box\\ammo_box";
		constexpr const char* RocketAmmo = "objects\\gear\\human\\military\\rocket_launcher_ammo\\rocket_launcher_ammo";
		constexpr const char* SniperAmmo = "objects\\gear\\human\\military\\sniper_rifle_ammo\\sniper_rifle_ammo";

		inline bool IsAmmo(const std::string& tag)
		{
			return tag == AmmoCabinet || tag == RocketAmmo || tag == SniperAmmo;
		}
	}

	namespace Spawn
	{
		constexpr const char* InvisibleRespawnPoint = "objects\\multi\\spawning\\respawn_point_invisible";
		constexpr const char* InitialSpawnPoint = "objects\\multi\\spawning\\initial_spawn_point";
		constexpr const char* RespawnPoint = "objects\\multi\\spawning\\respawn_point";

		inline bool IsSpawn(const std::string& tag)
		{
			return tag == InitialSpawnPoint || tag == RespawnPoint ||
				tag == InvisibleRespawnPoint;
		}

		inline bool IsInitialSpawn(const std::string& tag)
		{
			return tag == InitialSpawnPoint;
		}

		inline bool IsRespawn(const std::string& tag)
		{
			return tag == RespawnPoint;
		}

		inline bool IsInvisibleSpawn(const std::string& tag)
		{
			return tag == InvisibleRespawnPoint;
		}
	}

	namespace Boundary
	{
		constexpr const char* SafeBoundary = "objects\\multi\\boundaries\\safe_volume";
		constexpr const char* SoftSafeBoundary = "objects\\multi\\boundaries\\soft_safe_volume";
		constexpr const char* KillBoundary = "objects\\multi\\boundaries\\kill_volume";
		constexpr const char* SoftKillBoundary = "objects\\multi\\boundaries\\soft_kill_volume";

		inline bool IsBoundary(const std::string& tag)
		{
			return tag == SafeBoundary || tag == SoftSafeBoundary ||
				tag == KillBoundary || tag == SoftKillBoundary;
		}
	}

	namespace Explosive
	{
		constexpr const char* FusionCoil = "objects\\gear\\human\\military\\fusion_coil\\fusion_coil";
		constexpr const char* Landmine = "objects\\multi\\land_mine\\land_mine";
		constexpr const char* PlasmaBattery = "objects\\props\\covenant\\battery\\battery";
		constexpr const char* PropaneTank = "objects\\gear\\human\\industrial\\propane_tank\\propane_tank";

		inline bool IsExplosive(const std::string& tag)
		{
			return tag == FusionCoil || tag == Landmine ||
				tag == PlasmaBattery || tag == PropaneTank;
		}
	}

	namespace Lift
	{
		constexpr const char* ManCannon = "objects\\levels\\forge\\ff_man_cannon_forge\\ff_man_cannon_forge";
		constexpr const char* ManCannonHeavy = "objects\\levels\\forge\\ff_man_cannon_forge_heavy\\ff_man_cannon_forge_heavy";
		constexpr const char* ManCannonLight = "objects\\levels\\forge\\ff_man_cannon_forge_light\\ff_man_cannon_forge_light";
		constexpr const char* VehicleManCannon = "objects\\levels\\forge\\ff_veh_man_cannon\\ff_veh_man_cannon";
		constexpr const char* GravityLift = "objects\\levels\\forge\\ff_grav_lift\\ff_grav_lift";
		constexpr const char* ManCannonMCC = "objects\\multi\\dlc\\dlc_medium_mancannon\\dlc_medium_mancannon";
		constexpr const char* ManCannonHeavyMCC = "objects\\multi\\dlc\\dlc_medium_mancannon_heavy\\dlc_medium_mancannon_heavy";
		constexpr const char* ManCannonLightMCC = "objects\\multi\\dlc\\dlc_medium_mancannon_light\\dlc_medium_mancannon_light";
		constexpr const char* ForerunnerGravityLift = "objects\\cex\\cex_hangemhigh\\crates\\heh_gravlift_tunnels\\heh_gravlift_tunnels";
		constexpr const char* ForerunnerGravityLiftTall = "objects\\cex\\cex_hangemhigh\\crates\\heh_gravlift\\heh_gravlift";
		constexpr const char* ManCannonHumanMCC = "objects\\cex\\cex_headlong\\crates\\man_cannon\\man_cannon";

		inline bool IsLift(const std::string& tag)
		{
			return tag == ManCannon || tag == ManCannonHeavy ||
				tag == ManCannonLight || tag == VehicleManCannon ||
				tag == GravityLift || tag == ManCannonMCC ||
				tag == ManCannonHeavyMCC || tag == ManCannonLightMCC ||
				tag == ForerunnerGravityLift || tag == ForerunnerGravityLiftTall ||
				tag == ManCannonHumanMCC;
		}

		inline bool IsDefault(const std::string& tag)
		{
			return tag == ManCannon || ManCannonMCC ||
				tag == GravityLift || ForerunnerGravityLift;
		}

		inline bool IsHeavy(const std::string& tag)
		{
			return tag == ManCannonHeavy || tag == ManCannonHeavyMCC ||
				tag == ManCannonHumanMCC || ForerunnerGravityLiftTall;
		}

		inline bool IsLight(const std::string& tag)
		{
			return tag == ManCannonLight || tag == ManCannonLightMCC;
		}

		inline bool IsVehicle(const std::string& tag)
		{
			return tag == VehicleManCannon;
		}

		inline bool IsCurved(const std::string& tag)
		{
			return tag == ManCannon || tag == ManCannonHeavy ||
				tag == ManCannonLight || tag == ManCannonMCC ||
				tag == ManCannonHeavy || tag == ManCannonLightMCC ||
				tag == ManCannonHumanMCC;
		}

		inline bool IsVertical(const std::string tag)
		{
			return tag == GravityLift || tag == VehicleManCannon;
		}

		inline bool IsRedirected(const std::string& tag)
		{
			return tag == ForerunnerGravityLift || 
				tag == ForerunnerGravityLiftTall;
		}
	}

	namespace Objective
	{
		// CTF.
		constexpr const char* FlagStand = "objects\\multi\\models\\mp_flag_base\\mp_flag_base";
		constexpr const char* Flag = "objects\\weapons\\multiplayer\\flag\\flag";

		// Assault.
		constexpr const char* CapturePlate = "objects\\multi\\models\\mp_circle\\mp_circle";
		constexpr const char* Bomb = "objects\\weapons\\multiplayer\\assault_bomb\\assault_bomb";

		// Zone. (special case, still unsure on how to handle this)
		constexpr const char* HillMarker = "objects\\multi\\models\\mp_hill_beacon\\mp_hill_beacon";

		inline bool IsObjectiveSpawn(const std::string& tag)
		{
			return tag == FlagStand || tag == CapturePlate;
		}

		inline bool IsObjective(const std::string& tag)
		{
			return tag == Flag || tag == Bomb;
		}
	}

	namespace Shield
	{
		constexpr const char* OneWayDoorSmall = "objects\\levels\\forge\\ff_one_way_door_small\\ff_one_way_door_small";
		constexpr const char* OneWayDoorMedium = "objects\\levels\\forge\\ff_one_way_door_medium\\ff_one_way_door_medium";
		constexpr const char* OneWayDoorLarge = "objects\\levels\\forge\\ff_one_way_door_large\\ff_one_way_door_large";
		constexpr const char* TwoWayDoorSmall = "objects\\levels\\forge\\ff_two_way_door_small\\ff_two_way_door_small";
		constexpr const char* TwoWayDoorMedium = "objects\\levels\\forge\\ff_two_way_door_medium\\ff_two_way_door_medium";
		constexpr const char* TwoWayDoorLarge = "objects\\levels\\forge\\ff_two_way_door_large\\ff_two_way_door_large";
		constexpr const char* OneWayDoorXSmall = "objects\\levels\\forge\\ff_one_way_door_xsmall\\ff_one_way_door_xsmall";
		constexpr const char* OneWayDoorGarage = "objects\\levels\\forge\\ff_one_way_door_garage\\ff_one_way_door_garage";
		constexpr const char* ShieldWallSmall = "objects\\levels\\multi\\70_boneyard\\frigate_laser_field_small\\frigate_laser_field_small";
		constexpr const char* ShieldWallMedium = "objects\\levels\\multi\\70_boneyard\\frigate_laser_field_med\\frigate_laser_field_med";
		constexpr const char* ShieldWallLarge = "objects\\levels\\multi\\70_boneyard\\frigate_laser_field_wall\\frigate_laser_field_wall";
		constexpr const char* ShieldWallXLarge = "objects\\levels\\multi\\70_boneyard\\frigate_laser_field_gate\\frigate_laser_field_gate";
		constexpr const char* ShieldDoorSmall = "objects\\levels\\forge\\ff_shield_door_small\\ff_shield_door_small";
		constexpr const char* ShieldDoorMedium = "objects\\levels\\forge\\ff_shield_door_medium\\ff_shield_door_medium";
		constexpr const char* ShieldDoorLarge = "objects\\levels\\forge\\ff_shield_door_large\\ff_shield_door_large";
		constexpr const char* HangarShieldDoorSmall = "levels\\multi\\dlc\\objects\\dlc_slayer_shield_repair_s\\dlc_hangar_shield_repair_s\\dlc_hangar_shield_repair_s";
		constexpr const char* HangarShieldDoorSmallSolid = "levels\\multi\\dlc\\objects\\dlc_hangar_forcefield_small_solid\\dlc_hangar_forcefield_small_solid";
		constexpr const char* HangarShieldDoorLarge = "objects\\levels\\multi\\dlc\\dlc_slayer\\dlc_hangar_shield\\dlc_hangar_shield";
		constexpr const char* HangarShieldDoorLargeSolid = "levels\\multi\\dlc\\objects\\dlc_hangar_forcefield_solid\\dlc_hangar_forcefield_solid";

		constexpr const char* PortableShield = "objects\\props\\covenant\\cov_portable_shield\\cov_portable_shield";

		inline bool IsShield(const std::string& tag)
		{
			return tag == OneWayDoorSmall || tag == OneWayDoorMedium ||
				tag == OneWayDoorLarge || tag == TwoWayDoorSmall ||
				tag == TwoWayDoorMedium || tag == TwoWayDoorLarge ||
				tag == OneWayDoorXSmall || tag == OneWayDoorGarage ||
				tag == ShieldWallSmall || tag == ShieldWallMedium ||
				tag == ShieldWallLarge || tag == ShieldWallXLarge ||
				tag == ShieldDoorSmall || tag == ShieldDoorMedium ||
				tag == ShieldDoorLarge || tag == HangarShieldDoorSmall ||
				tag == HangarShieldDoorSmallSolid || tag == HangarShieldDoorLarge ||
				tag == HangarShieldDoorLargeSolid;
		}

		inline bool IsOneWay(const std::string& tag)
		{
			return tag == OneWayDoorSmall || tag == OneWayDoorMedium ||
				tag == OneWayDoorLarge || tag == OneWayDoorXSmall || 
				tag == OneWayDoorGarage || tag == ShieldDoorSmall ||
				tag == ShieldDoorMedium || tag == ShieldDoorLarge;
		}

		inline bool IsTwoWay(const std::string& tag)
		{
			return tag == TwoWayDoorSmall || tag == TwoWayDoorMedium || 
				tag == TwoWayDoorLarge || tag == HangarShieldDoorSmall ||
				tag == HangarShieldDoorLarge;
		}

		inline bool IsBlocker(const std::string& tag)
		{
			return tag == ShieldWallSmall || tag == ShieldWallMedium ||
				tag == ShieldWallLarge || tag == ShieldWallXLarge ||
				tag == HangarShieldDoorSmallSolid ||
				tag == HangarShieldDoorLargeSolid;
		}

		inline bool IsShieldDoor(const std::string& tag)
		{
			return tag == ShieldDoorSmall || tag == ShieldDoorMedium ||
				tag == ShieldDoorLarge;
		}

		inline bool IsPortableShield(const std::string& tag)
		{
			return tag == PortableShield;
		}
	}

	namespace Teleport
	{
		constexpr const char* TeleporterSender = "objects\\levels\\forge\\ff_teleporter_sender\\ff_teleporter_sender";
		constexpr const char* TeleporterReceiver = "objects\\levels\\forge\\ff_teleporter_receiver\\ff_teleporter_receiver";
		constexpr const char* TeleporterTwoWay = "objects\\levels\\forge\\ff_teleporter_2_way\\ff_teleporter_2_way";

		inline bool IsTeleport(const std::string& tag)
		{
			return tag == TeleporterSender || tag == TeleporterReceiver ||
				tag == TeleporterTwoWay;
		}

		inline bool IsReceiver(const std::string& tag)
		{
			return tag == TeleporterReceiver;
		}

		inline bool IsSender(const std::string& tag)
		{
			return tag == TeleporterSender;
		}

		inline bool IsTwoWay(const std::string& tag)
		{
			return tag == TeleporterTwoWay;
		}
	}

	namespace Weapon
	{
		constexpr const char* AssaultRifle = "objects\\weapons\\rifle\\assault_rifle\\assault_rifle";
		constexpr const char* DMR = "objects\\weapons\\rifle\\dmr\\dmr";
		constexpr const char* GrenadeLauncher = "objects\\weapons\\rifle\\grenade_rifle\\grenade_rifle";
		constexpr const char* Magnum = "objects\\weapons\\pistol\\magnum\\magnum";
		constexpr const char* RocktetLauncher = "objects\\weapons\\support_high\\rocket_launcher\\rocket_launcher";
		constexpr const char* Shotgun = "objects\\weapons\\rifle\\shotgun\\shotgun";
		constexpr const char* SniperRifle = "objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle";
		constexpr const char* SpartanLaser = "objects\\weapons\\support_high\\spartan_laser\\spartan_laser";
		constexpr const char* FragGrenade = "objects\\weapons\\grenade\\frag_grenade\\frag_grenade";
		constexpr const char* VehiMountedMachinegun = "objects\\vehicles\\human\\turrets\\machinegun\\machinegun";
		constexpr const char* MountedMachinegun = "objects\\vehicles\\human\\turrets\\machinegun\\weapon\\machinegun_turret_mounted\\machinegun_mounted";
		constexpr const char* Machinegun = "objects\\vehicles\\human\\turrets\\machinegun\\weapon\\machinegun_turret\\machinegun_turret";
		constexpr const char* ConcussionRifle = "objects\\weapons\\rifle\\concussion_rifle\\concussion_rifle";
		constexpr const char* EnergySword = "objects\\weapons\\melee\\energy_sword\\energy_sword";
		constexpr const char* FuelRod = "objects\\weapons\\support_high\\flak_cannon\\flak_cannon";
		constexpr const char* GravityHammer = "objects\\weapons\\melee\\gravity_hammer\\gravity_hammer";
		constexpr const char* FocusRifle = "objects\\weapons\\rifle\\focus_rifle\\focus_rifle";
		constexpr const char* NeedleRifle = "objects\\weapons\\rifle\\needle_rifle\\needle_rifle";
		constexpr const char* Needler = "objects\\weapons\\pistol\\needler\\needler";
		constexpr const char* PlasmaLauncher = "objects\\weapons\\support_high\\plasma_launcher\\plasma_launcher";
		constexpr const char* PlasmaPistol = "objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol";
		constexpr const char* PlasmaRepeater = "objects\\weapons\\rifle\\plasma_repeater\\plasma_repeater";
		constexpr const char* PlasmaRifle = "objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle";
		constexpr const char* Spiker = "objects\\weapons\\rifle\\spike_rifle\\spike_rifle";
		constexpr const char* PlasmaGrenade = "objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade";
		constexpr const char* VehiMountedPlasmaTurret = "objects\\vehicles\\covenant\\turrets\\plasma_turret\\plasma_turret_mounted";
		constexpr const char* MountedPlasmaTurret = "objects\\vehicles\\covenant\\turrets\\plasma_turret\\weapon\\plasma_turret_mounted\\plasma_turret_mounted";
		constexpr const char* PlasmaTurret = "objects\\vehicles\\covenant\\turrets\\plasma_turret\\weapon\\plasma_turret\\plasma_turret";

		inline bool IsWeapon(const std::string& tag)
		{
			return tag == AssaultRifle || tag == DMR ||
				tag == GrenadeLauncher || tag == Magnum ||
				tag == RocktetLauncher || tag == Shotgun ||
				tag == SniperRifle || tag == SpartanLaser ||
				tag == MountedMachinegun || tag == Machinegun ||
				tag == ConcussionRifle || tag == EnergySword || 
				tag == FuelRod || tag == GravityHammer || 
				tag == FocusRifle || tag == NeedleRifle ||
				tag == Needler || tag == PlasmaLauncher ||
				tag == PlasmaPistol || tag == PlasmaRepeater ||
				tag == PlasmaRifle || tag == Spiker ||
				tag == MountedPlasmaTurret || tag == PlasmaTurret;
		}

		inline bool IsGrenade(const std::string& tag)
		{
			return tag == FragGrenade || tag == PlasmaGrenade;
		}

		inline bool IsVehicle(const std::string tag)
		{
			return tag == VehiMountedPlasmaTurret || 
				tag == VehiMountedMachinegun;
		}
	}

	namespace Vehicle
	{
		constexpr const char* Banshee = "objects\\vehicles\\covenant\\banshee\\banshee";
		constexpr const char* Falcon = "objects\\vehicles\\human\\falcon\\falcon";
		constexpr const char* FalconSensor = "objects\\vehicles\\human\\falcon\\turrets\\falcon_sensor\\falcon_sensor";
		constexpr const char* FalconTurretRight = "objects\\vehicles\\human\\falcon\\turrets\\falcon_side_gun_right\\falcon_side_gun_right";
		constexpr const char* FalconTurretLeft = "objects\\vehicles\\human\\falcon\\turrets\\falcon_side_gun_left\\falcon_side_gun_left";
		constexpr const char* FalconGrenadeRight = "objects\\vehicles\\human\\falcon\\turrets\\falcon_side_grenade_right\\falcon_side_grenade_right";
		constexpr const char* FalconGrenadeLeft = "objects\\vehicles\\human\\falcon\\turrets\\falcon_side_grenade_left\\falcon_side_grenade_left";
		constexpr const char* FalconChinGun = "objects\\vehicles\\human\\falcon\\turrets\\falcon_chin_gun\\falcon_chin_gun";
		constexpr const char* Ghost = "objects\\vehicles\\covenant\\ghost\\ghost";
		constexpr const char* Mongoose = "objects\\vehicles\\human\\mongoose\\mongoose";
		constexpr const char* Revenant = "objects\\vehicles\\covenant\\revenant\\revenant";
		constexpr const char* RevenantPlasmaTurret = "objects\\vehicles\\covenant\\revenant\\turrets\\revenant_plasma_turret\\revenant_plasma_turret";
		constexpr const char* Scorpion = "objects\\vehicles\\human\\scorpion\\scorpion";
		constexpr const char* ScorpionTurret = "objects\\vehicles\\human\\scorpion\\turrets\\scorpion_anti_infantry\\scorpion_anti_infantry";
		constexpr const char* ScorpionCannon = "objects\\vehicles\\human\\scorpion\\turrets\\scorpion_cannon\\scorpion_cannon";
		constexpr const char* Shade = "objects\\vehicles\\covenant\\turrets\\shade\\shade";
		constexpr const char* ShadePlasmaCannon = "objects\\vehicles\\covenant\\turrets\\shade\\weapons\\shade_plasma_cannon\\shade_plasma_cannon";
		constexpr const char* ShadeFlakCannon = "objects\\vehicles\\covenant\\turrets\\shade\\weapons\\shade_flak_cannon\\shade_flak_cannon";
		constexpr const char* Warthog = "objects\\vehicles\\human\\warthog\\warthog";
		constexpr const char* WarthogChaingun = "objects\\vehicles\\human\\warthog\\weapons\\warthog_chaingun\\warthog_chaingun";
		constexpr const char* WarthogGauss = "objects\\vehicles\\human\\warthog\\weapons\\warthog_gauss\\warthog_gauss";
		constexpr const char* WarthogRocket = "objects\\vehicles\\human\\warthog\\weapons\\warthog_rocket\\warthog_rocket";
		constexpr const char* WarthogTroop = "objects\\vehicles\\human\\warthog\\weapons\\warthog_troop\\warthog_troop";
		constexpr const char* Wraith = "objects\\vehicles\\covenant\\wraith\\wraith";
		constexpr const char* WraithPlasmaTurret = "objects\\vehicles\\covenant\\wraith\\turrets\\wraith_anti_infantry\\wraith_anti_infantry";
		constexpr const char* WraithCannon = "objects\\vehicles\\covenant\\wraith\\turrets\\wraith_mortar\\wraith_mortar";
		constexpr const char* Sabre = "objects\\vehicles\\human\\sabre\\sabre";
		constexpr const char* Seraph = "objects\\vehicles\\covenant\\seraph\\seraph";
		constexpr const char* CartElectric = "objects\\vehicles\\human\\civilian\\cart_electric\\cart_electric";
		constexpr const char* Forklift = "objects\\vehicles\\human\\civilian\\forklift\\forklift";
		constexpr const char* Pickup = "objects\\vehicles\\human\\civilian\\pickup\\pickup";
		constexpr const char* TruckCab = "objects\\vehicles\\human\\civilian\\truck_cab_large\\truck_cab_large";
		constexpr const char* TruckCabBedLong = "objects\\vehicles\\human\\civilian\\truck_cab_large\\attachments\\bed_long\\bed_long";
		constexpr const char* OniVan = "objects\\vehicles\\human\\civilian\\oni_van\\oni_van";

		inline bool IsVehicle(const std::string& tag)
		{
			return 
				tag == Banshee || 
				tag == Falcon || tag == FalconSensor ||
				tag == FalconTurretRight || tag == FalconTurretLeft ||
				tag == FalconGrenadeRight || tag == FalconGrenadeLeft ||
				tag == FalconChinGun ||
				tag == Ghost || 
				tag == Mongoose ||
				tag == Revenant || 
				tag == RevenantPlasmaTurret ||
				tag == Scorpion || tag == ScorpionTurret || 
				tag == ScorpionCannon ||
				tag == Shade || tag == ShadePlasmaCannon ||
				tag == ShadeFlakCannon ||
				tag == Warthog || tag == WarthogChaingun ||
				tag == WarthogGauss || tag == WarthogRocket ||
				tag == WarthogTroop ||
				tag == Wraith || tag == WraithPlasmaTurret ||
				tag == WraithCannon ||
				tag == Sabre || 
				tag == Seraph ||
				tag == CartElectric || 
				tag == Forklift || 
				tag == Pickup ||
				tag == TruckCab || tag == TruckCabBedLong ||
				tag == OniVan;
		}

		inline bool HasBoost(const std::string& tag)
		{
			return tag == Banshee || tag == Ghost ||
				tag == Wraith || tag == Revenant;
		}
	}

	namespace Palletes
	{
		constexpr const char* Pallet = "objects\\gear\\human\\industrial\\pallet\\pallet";
		constexpr const char* PalletLarge = "objects\\gear\\human\\industrial\\pallet_large\\pallet_large";

		inline bool IsPallete(const std::string& tag)
		{
			return tag == Pallet || tag == PalletLarge;
		}
	}

	namespace ControlDevice
	{
		constexpr const char* HealthStation = "objects\\devices\\human\\health_station\\health_station";

		inline bool IsHealthStation(const std::string& tag)
		{
			return tag == HealthStation;
		}
	}

	namespace SkyBox
	{
		constexpr const char* ForgeWorld = "levels\\multi\\forge_halo\\sky_halo\\sky_halo";
		constexpr const char* WinterContingency = "levels\\solo\\m10\\sky_morning_overcast\\sky_morning_overcast";
		constexpr const char* OniSwordBase = "levels\\solo\\m20\\sky_morning\\sky_morning";
		constexpr const char* Nightfall = "levels\\solo\\m30\\sky_night\\sky_night";
		constexpr const char* TipOfTheSpear = "levels\\solo\\m35\\sky_daytime\\sky_daytime";

		inline bool IsSkyBox(const std::string& tag)
		{
			return tag == ForgeWorld || tag == WinterContingency ||
				tag == OniSwordBase || tag == Nightfall ||
				tag == TipOfTheSpear;
		}
	}
}