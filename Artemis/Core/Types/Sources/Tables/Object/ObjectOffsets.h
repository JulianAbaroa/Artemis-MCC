#pragma once

#include <cstdint>

// TODO: Add all the vehicles and their data.
// TODO: See if there's a size/offset value for vehicle seats.

namespace ObjectOffsets
{
	constexpr uintptr_t DatumIndex = 0x000;						// uint32.
	constexpr uintptr_t NextSiblingHandle = 0x00C;				// uint32.
	constexpr uintptr_t ChildHandle = 0x010;					// uint32.
	constexpr uintptr_t ParentHandle = 0x014;					// uint32.
	constexpr uintptr_t UnknownVariant = 0x1C;					// uint8.
	constexpr uintptr_t CurrentPosition = 0x020;				// float (3).
	constexpr uintptr_t CurrentRadius = 0x02C;					// float (1).
	constexpr uintptr_t Forward = 0x050;						// float (3).
	constexpr uintptr_t Up = 0x05C;								// float (3).
	constexpr uintptr_t LinearVelocity = 0x068;					// float (3).
	constexpr uintptr_t AngularVelocity = 0x074;				// float (3).
	constexpr uintptr_t HlmtVariant = 0xEA;						// uint8.
	constexpr uintptr_t OwnerPlayerHandle = 0xF0;				// uint32.
	constexpr uintptr_t OwnerBipedHandle = 0xF4;				// uint32.
	constexpr uintptr_t DamageReceived = 0x128;					// float (1).
	constexpr uintptr_t DamageRegionsSize = 0x184;				// uint16.
	constexpr uintptr_t DamageRegionsOffset = 0x186;			// uint16.

	namespace Biped
	{
		constexpr uintptr_t VerticalState = 0x1D8;				// uint8.
		constexpr uintptr_t IsAbilityActive = 0x1D9;			// uint8.
		constexpr uintptr_t MovementDirection = 0x260;			// float (2).
		constexpr uintptr_t ZoomLevel = 0x383;					// uint8.
		constexpr uintptr_t GroundObjectHandle = 0xAD0;			// uint32.
		constexpr uintptr_t MaterialIndex = 0xAD4;				// uint16.
		constexpr uintptr_t IsGrounded = 0xAD7;					// uint8.
		constexpr uintptr_t SurfaceNormal = 0xAD8;				// float (3).
		constexpr uintptr_t DamagerBipedHandle = 0x4E8;			// uint32.
		constexpr uintptr_t DamagerPlayerHandle = 0x4EC;		// uint32.
	}

	namespace Vehicle
	{
		constexpr uintptr_t BoostThrottle = 0x53C;				// float (1).
		constexpr uintptr_t BoostEnergy = 0x540;				// float (1).
		constexpr uintptr_t BoostCooldown = 0x544;				// float (1).

		namespace Banshee
		{
			constexpr uintptr_t WeaponMode = 0x34A;				// uint8.
			constexpr uintptr_t DriverSeat = 0x2718;			// float (1).
			constexpr uintptr_t DriverRightHijacker = 0x2724;	// float (1).
			constexpr uintptr_t DriverLeftHijacker = 0x2730;	// float (1).
			constexpr uintptr_t HasActiveBomb = 0x2B9A;			// uint8.
			constexpr uintptr_t IsCannonDepleted = 0x2DE6;		// uint8.
			constexpr uintptr_t IsFiring = 0x29E1;				// uint8.
		}

		namespace Falcon
		{
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
			constexpr uintptr_t DriverSeat = 0x2418;				// float (1).
			constexpr uintptr_t BackDriverHijacker = 0x2424;		// float (1).
			constexpr uintptr_t FrontRightDriverHijacker = 0x2430;	// float (1).
			constexpr uintptr_t FrontLeftDriverHijacker = 0x243C;	// float (1).
			constexpr uintptr_t IsFiring = 0x2771;					// uint8.
		}

		namespace Mongoose
		{
			constexpr uintptr_t DriverSeat = 0x263C;			// float (1).
			constexpr uintptr_t PassengerSeat = 0x2648;			// float (1).
			constexpr uintptr_t DriverHijacker = 0x2654;		// float (1).
			constexpr uintptr_t PassengerHijacker = 0x2660;		// float (1).
		}

		namespace Revenant
		{
			constexpr uintptr_t DriverSeat = 0x2118;			// float (1).	
			constexpr uintptr_t PassengerSeat = 0x2124;			// float (1).
			constexpr uintptr_t DriverHijacker = 0x2130;		// float (1).
			constexpr uintptr_t PassengerHijacker = 0x213C;		// float (1).
		}

		namespace Scorpion
		{
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
			constexpr uintptr_t GunnerSeat = 0x3960;		// float (1).	
			// TODO: Get IsFiring.
		}

		namespace Warthog
		{
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
	}
}