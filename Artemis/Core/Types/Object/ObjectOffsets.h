#pragma once

#include <cstdint>

// TODO: Find the team of the objectives (Flag, Bomb, etc).
// TODO: Maybe get the health of turrets, cannons and such.
// TODO: Add all the vehicles and their data.

// --- Object ---
// DatumIndex: The handle of the specific object type.
// Next Sibling Handle: The handle of the next sibling object.
// Child Handle: The handle of the child object.
// Parent Handle: The handle of the parent object.
// Current Position: The current position of the object in world-space.
// Current Radius: The current radius of the object.
// Forward: The forward of this object in world-space.
// Up: The up of this object in world-space.
// Linear Velocity: The linear velocity of this object.
// Angular Velocity: The angular velocity of this object.
// Owner Player Handle: The handle of the last player that interacted with this object.
// Owner Biped Handle: The handle of the last biped that interacted with this object.
// Damage Recieved: The last health damage that this object recieved. (TODO)

// --- Biped ---
// Is Crouched: Represents if the biped is crouched or not. (TODO)
// Is Ability Active: Its behaviour depends on the armor ability. (TODO)
// Movement Direction: The movement direction of this biped in local-space. (X: 1.0 == East, -1.0 == West, Y: 1.0 == South, -1.0 == North)
// Zoom Level: The current zoom of this biped. (None, x1, x2)
// Ground Object Handle: The handle of the object this biped is standing on. (0xFFFF == InAir)
// Material Index: The material index of the surface this biped is standing on.
// Is Grounded: Represents if this surface is walkeable (probably used for NCPs, since even if its zero, players still can walk on it)
// Surface normal: The surface normal of the surface this biped is standing on.
// Damager Biped Handle: The handle of the biped that just damaged this biped.
// Damager Player Handle: The handlee of the player that just damaged this biped.
// Regeneration Delay: The amount of frames(*) until the health can start regenerating. (TODO)
// Health: The health of this biped.
// Has No Shields: Represents if this biped has shields.
// Shields: The shields of this biped.

// --- Vehicle ---
// 
// Seats note: Is 1.0f when free, anything else is occupied. 
// Probably it shows the animation progress, but for specific vehicles
// and specific seats, like for the driver seat in falcon.
// 
// Boost Throttle: Represents the boost speed (min/max), clamped between 0.0 and 1.0.
// Boost Energy: Represents the amount of boost energy this vehicle currently has.
// Boost Cooldown: Represents when this vehicle is able to recharge the boost energy.
// Weapon Mode: Represents the selected weapon. (banshee: 0x00: dual-cannon, 0x01: single-cannon)
// Regeneration Delay: ...
// Health: The health of this vehicle.
// 
// I'm going to skip seats...
// 
// Has Active Bomb: Represents if this vehicle has at least one active bomb.
// Is Cannon Depleted: Represents if the cannon of this vehicle is depleted. (0x00 when charged, 0x01 when uncharged)
// Is Firing: Represents if this vehicle is currently firing.

// --- Weapon ---
// Action State: Represents the current state of this weapon. 
// Total Heat: Represents how current heat of this weapon. (1.0 == overheated)
// Total Energy: Represents the current energy of this weapon. (ammo)
// Is Tracking: Represents if this weapon is tracking another biped. (0x01 == IsTracking/Crosshair turns red)
// Tracked Biped Handle: The handle of the tracked biped.
// Is Firing: Represents the shoot trigger state. (0x01 == player is shooting)
// Charge Progress: Represents the charge progress of this weapon projectile. (when it reaches zero, is fully charged)
// Is Reloading: Represents the reloading state of this weapon. (0x01 == player is reloading)			
// Total Ammo: Represents the total ammo of this weapon.	
// Current Ammo: Represents the currnet ammo of this weapon. (from magazine)	

// --- Projectile ---
// Runtime Flags: 0x100 == IsTracking, 0x400000 == IsCritical, 0x800000 == IsAltFire. (TODO)
// Owner Biped Handle: The handle of the biped that had the weapon that shooted this projectile.
// Owner Weapon Handle: The handle of the weapon that shooted this projectile.

// --- Equipment ---
// Total Energy: Represents the current energy of this armor ability. (1.0 == full)

// --- Crate ---
// Radius: The radius of the shape of this object.	
// Lenght: The length of the shape of this object.	
// Top: The top of the shape of this object.	
// Bottom: The bottom of the shape of this object.	
// ShapeType: The shape type of this object. (none, cylinder, box)
// Team: The team of this object.
// 
// -- Teleporter --
// 
// Channel: The channel of this teleporter.
// Allowed Objects: The allowed objects to teleport through this teleport.
//
// Regeneration Delay: ...
// Health: The health of this object.
// Team: ...

namespace ObjectOffsets
{
	constexpr uintptr_t DatumIndex = 0x000;						// uint32.
	constexpr uintptr_t NextSiblingHandle = 0x00C;				// uint32.
	constexpr uintptr_t ChildHandle = 0x010;					// uint32.
	constexpr uintptr_t ParentHandle = 0x014;					// uint32.
	constexpr uintptr_t CurrentPosition = 0x020;				// float (3).
	constexpr uintptr_t CurrentRadius = 0x02C;					// float (1).
	constexpr uintptr_t Forward = 0x050;						// float (3).
	constexpr uintptr_t Up = 0x05C;								// float (3).
	constexpr uintptr_t LinearVelocity = 0x068;					// float (3).
	constexpr uintptr_t AngularVelocity = 0x074;				// float (3).
	constexpr uintptr_t OwnerPlayerHandle = 0xF0;				// uint32.
	constexpr uintptr_t OwnerBipedHandle = 0xF4;				// uint32.
	constexpr uintptr_t DamageReceived = 0x128;					// float (1).

	namespace Biped
	{
		// IsCrouched: 0x01, IsJumping: 0x02, IsJumpingCrouched: 0x03
		constexpr uintptr_t IsCrouched = 0x1D8;					// uint8.
		constexpr uintptr_t IsAbilityActive = 0x1D9;			// uint8.
		constexpr uintptr_t MovementDirection = 0x260;			// float (2).
		constexpr uintptr_t ZoomLevel = 0x383;					// uint8.
		constexpr uintptr_t GroundObjectHandle = 0xAD0;			// uint32.
		constexpr uintptr_t MaterialIndex = 0xAD4;				// uint16.
		constexpr uintptr_t IsGrounded = 0xAD7;					// uint8.
		constexpr uintptr_t SurfaceNormal = 0xAD8;				// float (3).
		constexpr uintptr_t DamagerBipedHandle = 0x4E8;			// uint32.
		constexpr uintptr_t DamagerPlayerHandle = 0x4EC;		// uint32.

		namespace Spartan
		{
			constexpr uintptr_t HealthRegenerationDelay = 0xE48;	// uint16.
			constexpr uintptr_t Health = 0xE4C;						// float (1).
			constexpr uintptr_t HasNoShields = 0xE6C;				// uint8.
			constexpr uintptr_t ShieldsRegenerationDelay = 0xE78;	// uint16.
			constexpr uintptr_t Shields = 0xE7C;					// float (1).
		}

		namespace Elite
		{
			constexpr uintptr_t HealthRegenerationDelay = 0xE40;	// uint16.
			constexpr uintptr_t Health = 0xE44;						// float (1).
			constexpr uintptr_t HasNoShields = 0xE64;				// uint8.
			constexpr uintptr_t ShieldsRegenerationDelay = 0xE70;	// uint16.
			constexpr uintptr_t Shields = 0xE74;					// float (1).
		}
	}

	namespace Vehicle
	{
		constexpr uintptr_t BoostThrottle = 0x53C;				// float (1).
		constexpr uintptr_t BoostEnergy = 0x540;				// float (1).
		constexpr uintptr_t BoostCooldown = 0x544;				// float (1).

		namespace Banshee
		{
			constexpr uintptr_t WeaponMode = 0x34A;				// uint8.
			constexpr uintptr_t RegenerationDelay = 0xE48;		// uint16.
			constexpr uintptr_t Health = 0xE4C;					// float (1).
			constexpr uintptr_t DriverSeat = 0x2718;			// float (1).
			constexpr uintptr_t DriverRightHijacker = 0x2724;	// float (1).
			constexpr uintptr_t DriverLeftHijacker = 0x2730;	// float (1).
			constexpr uintptr_t HasActiveBomb = 0x2B9A;			// uint8.
			constexpr uintptr_t IsCannonDepleted = 0x2DE6;		// uint8.
			constexpr uintptr_t IsFiring = 0x29E1;				// uint8.
		}

		namespace Falcon
		{
			constexpr uintptr_t RegeneratitionDelay = 0xEA8;		// uint16
			constexpr uintptr_t Health = 0xEAC;						// float (1).
			constexpr uintptr_t DriverSeat = 0x27DC;				// float (1).
			constexpr uintptr_t FrontLeftPassengerSeat = 0x27E8;	// float (1).
			constexpr uintptr_t FrontRightPassengerSeat = 0x27F4;	// float (1).
			constexpr uintptr_t BackLeftPassengerSeat = 0x2800;		// float (1).
			constexpr uintptr_t BackRightPassengerSeat = 0x280C;	// float (1).
			constexpr uintptr_t BackMiddlePassengerSeat = 0x2818;	// float (1).
			constexpr uintptr_t LeftDriverHijacker = 0x2824;		// float (1).
			constexpr uintptr_t RightDriverHijacker = 0x2830;		// float (1).

			namespace Turret
			{
				constexpr uintptr_t GunnerSeat = 0x2268;			// float (1).
				constexpr uintptr_t GunnerHijacker = 0x2274;		// float (1).
				// TODO: Get IsFiring.
			}

			namespace Grenadier
			{
				constexpr uintptr_t GrenadierSeat = 0x22D4;			// float (1).
				constexpr uintptr_t GrenadierHijacker = 0x228C;		// float (1).
				// TODO: Get IsFiring.
			}
		}

		namespace Ghost
		{
			constexpr uintptr_t RegeneratitionDelay = 0xE50;		// uint16.
			constexpr uintptr_t Health = 0xE54;						// float (1).
			constexpr uintptr_t DriverSeat = 0x2418;				// float (1).
			constexpr uintptr_t BackDriverHijacker = 0x2424;		// float (1).
			constexpr uintptr_t FrontRightDriverHijacker = 0x2430;	// float (1).
			constexpr uintptr_t FrontLeftDriverHijacker = 0x243C;	// float (1).
			constexpr uintptr_t IsFiring = 0x2771;					// uint8.
		}

		namespace Mongoose
		{
			constexpr uintptr_t RegenerationDelay = 0xE28;		// uint16.
			constexpr uintptr_t Health = 0xE2C;					// float (1).
			constexpr uintptr_t DriverSeat = 0x263C;			// float (1).
			constexpr uintptr_t PassengerSeat = 0x2648;			// float (1).
			constexpr uintptr_t DriverHijacker = 0x2654;		// float (1).
			constexpr uintptr_t PassengerHijacker = 0x2660;		// float (1).
		}

		namespace Revenant
		{
			constexpr uintptr_t RegenerationDelay = 0xE38;		// uint16.
			constexpr uintptr_t Health = 0xE3C;					// float (1).
			constexpr uintptr_t DriverSeat = 0x2118;			// float (1).	
			constexpr uintptr_t PassengerSeat = 0x2124;			// float (1).
			constexpr uintptr_t DriverHijacker = 0x2130;		// float (1).
			constexpr uintptr_t PassengerHijacker = 0x213C;		// float (1).
		}

		namespace Scorpion
		{
			constexpr uintptr_t RegenerationDelay = 0xE40;		// uint16.
			constexpr uintptr_t Health = 0xE44;					// float (1).
			constexpr uintptr_t DriverSeat = 0x27E0;			// float (1).	
			constexpr uintptr_t RightDriverHijacker = 0x281C;	// float (1).	
			constexpr uintptr_t LeftDriverHijacker = 0x2828;	// float (1).	
			constexpr uintptr_t BackHijacker = 0x2834;			// float (1).	

			namespace Turret
			{
				constexpr uintptr_t GunnerSeat = 0x1E90;		// float (1).	
				// TODO: Get IsFiring.
			}
		}

		namespace ShadeTurret
		{
			constexpr uintptr_t RegenerationDelay = 0xDF4;	// uint16.
			constexpr uintptr_t Health = 0xDF8;				// float (1).
			constexpr uintptr_t GunnerSeat = 0x3960;		// float (1).	
			// TODO: Get IsFiring.
		}

		namespace Warthog
		{
			constexpr uintptr_t RegenerationDelay = 0xE78;	// uint16.
			constexpr uintptr_t Health = 0xE7C;				// float (1).
			constexpr uintptr_t DriverSeat = 0x2AB0;		// float (1).	
			constexpr uintptr_t PassengerSeat = 0x2ABC;		// float (1).	
			constexpr uintptr_t DriverHijacker = 0x2AC8;	// float (1).	
			constexpr uintptr_t PassengerHijacker = 0x2AD4;	// float (1).	

			namespace Turret
			{
				constexpr uintptr_t GunnerSeat = 0x1F48;			// float (1).	
				constexpr uintptr_t GunnerHijackerSeat = 0x1F54;	// float (1).	
				// TODO: Get IsFiring.
			}

			namespace Gauss
			{
				constexpr uintptr_t GunnerSeat = 0x1F38;			// float (1).	
				constexpr uintptr_t GunnerHijackerSeat = 0x1F44;	// float (1).	
			}

			namespace Rocket
			{
				constexpr uintptr_t GunnerSeat = 0x2020;			// float (1).	
				constexpr uintptr_t GunnerHijackerSeat = 0x202C;	// float (1).	
			}
		}

		namespace Wraith
		{
			constexpr uintptr_t RegenerationDelay = 0xE48;			// uint16.
			constexpr uintptr_t Health = 0xE4C;						// float (1).
			constexpr uintptr_t DriverSeat = 0x23E4;				// float (1).	
			constexpr uintptr_t LeftDriverHijackerSeat = 0x23F0;	// float (1).	
			constexpr uintptr_t RightDriverHijackerSeat = 0x23FC;	// float (1).	
			constexpr uintptr_t BackHijackerSeat = 0x2408;			// float (1).	

			namespace PlasmaTurret
			{
				constexpr uintptr_t GunnerSeat = 0x1EC4;	// float (1).	
				// TODO: Get IsFiring.
			}
		}
	}

	namespace Weapon
	{
		constexpr uintptr_t ActionState = 0x1CA;		// uint8.
		constexpr uintptr_t TotalHeat = 0x1E0;			// float (1).
		constexpr uintptr_t TotalEnergy = 0x1E4;		// float (1).
		constexpr uintptr_t IsTracking = 0x1F8;			// uint8.
		constexpr uintptr_t TrackedBipedHandle = 0x200;	// uint32.
		constexpr uintptr_t IsFiring = 0x2A8;			// uint8.
		constexpr uintptr_t ChargeProgress = 0x2AC;		// uint8.	
		constexpr uintptr_t IsReloading = 0x2C0;		// uint8.
		constexpr uintptr_t TotalAmmo = 0x2C6;			// uint16.
		constexpr uintptr_t CurrentAmmo = 0x2CA;		// uint16.

		namespace Flag
		{
			constexpr uintptr_t Team = 0x6D1;	// uint8.
		}
		
		namespace Bomb
		{
			constexpr uintptr_t Team = 0x6E5;	// uint8.
		}
	}

	namespace Projectile
	{
		constexpr uintptr_t RuntimeFlags = 0x1A8;		// uint32.
		constexpr uintptr_t OwnerBipedHandle = 0x1C4;	// uint32.
		constexpr uintptr_t OwnerWeaponHandle = 0x1D8;	// uint32.
	}

	namespace Equipment
	{
		constexpr uintptr_t TotalEnergy = 0x1EC;	// float (1).
	}

	namespace Crate
	{
		// Just for Flag & Capture Plate.
		namespace ObjectiveSpawn
		{
			constexpr uintptr_t Radius = 0x258;		// float (1).
			constexpr uintptr_t Lenght = 0x25C;		// float (1).
			constexpr uintptr_t Top = 0x260;		// float (1).
			constexpr uintptr_t Bottom = 0x264;		// float (1).
			constexpr uintptr_t ShapeType = 0x268;	// uint8.
			constexpr uintptr_t Team = 0x271;		// uint8.
		}

		namespace HillMarker
		{
			constexpr uintptr_t Radius = 0x274;		// float (1).
			constexpr uintptr_t Lenght = 0x278;		// float (1).
			constexpr uintptr_t Top = 0x27C;		// float (1).
			constexpr uintptr_t Bottom = 0x280;		// float (1).
			constexpr uintptr_t ShapeType = 0x284;	// uint8.
			constexpr uintptr_t Team = 0x28D;		// uint8.
		}

		namespace Teleporter
		{
			constexpr uintptr_t Radius = 0x290;		// float (1).
			constexpr uintptr_t Lenght = 0x294;		// float (1).
			constexpr uintptr_t Top = 0x298;		// float (1).
			constexpr uintptr_t Bottom = 0x29C;		// float (1).
			constexpr uintptr_t ShapeType = 0x2A0;	// uint8.
			constexpr uintptr_t Channel = 0x2A5;	// uint8.
			constexpr uintptr_t AllowedObjects = 0x2A6; // uint8.
		}

		namespace FusionCoil
		{
			constexpr uintptr_t RegenerationDelay = 0x1C8;	// uint16.
			constexpr uintptr_t Health = 0x1CC;				// float (1).
		}

		namespace Landmine
		{
			constexpr uintptr_t Health = 0x1CC;	// float (1).
		}

		namespace PlasmaBattery
		{
			constexpr uintptr_t Health = 0x1CC;	// float (1).
		}

		namespace PropaneTank
		{
			constexpr uintptr_t Health = 0x1C0;	// float (1).
		}

		namespace Palletes
		{
			constexpr uintptr_t Health = 0x1C0;	// float (1).
		}
	}

	namespace Scenery
	{
		// Initial & Respawn.
		namespace SpawnPoint
		{
			constexpr uintptr_t Team = 0x2C1;	// uint8.
		}
		
		// Safe, SoftSafe, Kill & SoftKill.
		namespace Boundary
		{
			constexpr uintptr_t Team = 0x2A5;		// uint8.
			constexpr uintptr_t Radius = 0x28C;		// float (1).
			constexpr uintptr_t Lenght = 0x290;		// float (1).
			constexpr uintptr_t Top = 0x294;		// float (1).
			constexpr uintptr_t Bottom = 0x298;		// float (1).
			constexpr uintptr_t ShapeType = 0x29C;	// uint8.
		}

		namespace PortableShield
		{
			constexpr uintptr_t RegenerationDelay = 0x21C;	// uint16.
			constexpr uintptr_t Health = 0x220;	// 1 float.
		}
	}
}

/*

	FUN_1801fbd40 -> FUN_1801fc5cc, FUN_18017db0c, FUN_1801fc294 
	FUN_1801936e8
	FUN_1804eb5c0 (Update Biped) -> FUN_1804e9c6c -> FUN_1801947e8
	FUN_18053a5c4

	FUN_18016eb14 <- physics (FUN_1800bc920, FUN_1801177a0, FUN_180117dc8, FUN_180117d20, AllocateAndEnqueueWorkItem) FUN_18009c420 

	PTR_FUN_180996c58 <- raycasts.

	FUN_1800990f0
	FUN_18013085c <- importante.
	LAB_180099084

*/