export module Tables.Object.Type:Offset;

import std;

// Offsets of the object datum that have no counterpart in Template.Object.Type yet.
// note: Everything inside the fixed part of a datum was moved to Template.Object.Type. What is left lives in
// the variable region after it (seats, shapes, teams), pending the generated block descriptors.

export namespace Tables::Object::Type::Offset
{
	namespace Vehicle
	{
		namespace Banshee
		{
			constexpr std::uintptr_t DriverSeat{ 0x2718 };			// float (1).
			constexpr std::uintptr_t DriverRightHijacker{ 0x2724 };	// float (1).
			constexpr std::uintptr_t DriverLeftHijacker{ 0x2730 };	// float (1).
			constexpr std::uintptr_t HasActiveBomb{ 0x2B9A };		// uint8.
			constexpr std::uintptr_t IsCannonDepleted{ 0x2DE6 };	// uint8.
			constexpr std::uintptr_t IsFiring{ 0x29E1 };			// uint8.
		}

		namespace Falcon
		{
			constexpr std::uintptr_t DriverSeat{ 0x27DC };				// float (1).
			constexpr std::uintptr_t FrontLeftPassengerSeat{ 0x27E8 };	// float (1).
			constexpr std::uintptr_t FrontRightPassengerSeat{ 0x27F4 };	// float (1).
			constexpr std::uintptr_t BackLeftPassengerSeat{ 0x2800 };	// float (1).
			constexpr std::uintptr_t BackRightPassengerSeat{ 0x280C };	// float (1).
			constexpr std::uintptr_t BackMiddlePassengerSeat{ 0x2818 };	// float (1).
			constexpr std::uintptr_t LeftDriverHijacker{ 0x2824 };		// float (1).
			constexpr std::uintptr_t RightDriverHijacker{ 0x2830 };		// float (1).

			namespace Turret
			{
				constexpr std::uintptr_t GunnerSeat{ 0x2268 };		// float (1).
				constexpr std::uintptr_t GunnerHijacker{ 0x2274 };	// float (1).
				// TODO: Get IsFiring.
			}

			namespace Grenadier
			{
				constexpr std::uintptr_t GrenadierSeat{ 0x22D4 };		// float (1).
				constexpr std::uintptr_t GrenadierHijacker{ 0x228C };	// float (1).
				// TODO: Get IsFiring.
			}
		}

		namespace Ghost
		{
			constexpr std::uintptr_t DriverSeat{ 0x2418 };				// float (1).
			constexpr std::uintptr_t BackDriverHijacker{ 0x2424 };		// float (1).
			constexpr std::uintptr_t FrontRightDriverHijacker{ 0x2430 };// float (1).
			constexpr std::uintptr_t FrontLeftDriverHijacker{ 0x243C };	// float (1).
			constexpr std::uintptr_t IsFiring{ 0x2771 };				// uint8.
		}

		namespace Mongoose
		{
			constexpr std::uintptr_t DriverSeat{ 0x263C };			// float (1).
			constexpr std::uintptr_t PassengerSeat{ 0x2648 };		// float (1).
			constexpr std::uintptr_t DriverHijacker{ 0x2654 };		// float (1).
			constexpr std::uintptr_t PassengerHijacker{ 0x2660 };	// float (1).
		}

		namespace Revenant
		{
			constexpr std::uintptr_t DriverSeat{ 0x2118 };			// float (1).
			constexpr std::uintptr_t PassengerSeat{ 0x2124 };		// float (1).
			constexpr std::uintptr_t DriverHijacker{ 0x2130 };		// float (1).
			constexpr std::uintptr_t PassengerHijacker{ 0x213C };	// float (1).
		}

		namespace Scorpion
		{
			constexpr std::uintptr_t DriverSeat{ 0x27E0 };			// float (1).
			constexpr std::uintptr_t RightDriverHijacker{ 0x281C };	// float (1).
			constexpr std::uintptr_t LeftDriverHijacker{ 0x2828 };	// float (1).
			constexpr std::uintptr_t BackHijacker{ 0x2834 };		// float (1).

			namespace Turret
			{
				constexpr std::uintptr_t GunnerSeat{ 0x1E90 };	// float (1).
				// TODO: Get IsFiring.
			}
		}

		namespace ShadeTurret
		{
			constexpr std::uintptr_t GunnerSeat{ 0x3960 };	// float (1).
			// TODO: Get IsFiring.
		}

		namespace Warthog
		{
			constexpr std::uintptr_t DriverSeat{ 0x2AB0 };			// float (1).
			constexpr std::uintptr_t PassengerSeat{ 0x2ABC };		// float (1).
			constexpr std::uintptr_t DriverHijacker{ 0x2AC8 };		// float (1).
			constexpr std::uintptr_t PassengerHijacker{ 0x2AD4 };	// float (1).

			namespace Turret
			{
				constexpr std::uintptr_t GunnerSeat{ 0x1F48 };			// float (1).
				constexpr std::uintptr_t GunnerHijackerSeat{ 0x1F54 };	// float (1).
				// TODO: Get IsFiring.
			}

			namespace Gauss
			{
				constexpr std::uintptr_t GunnerSeat{ 0x1F38 };			// float (1).
				constexpr std::uintptr_t GunnerHijackerSeat{ 0x1F44 };	// float (1).
			}

			namespace Rocket
			{
				constexpr std::uintptr_t GunnerSeat{ 0x2020 };			// float (1).
				constexpr std::uintptr_t GunnerHijackerSeat{ 0x202C };	// float (1).
			}
		}

		namespace Wraith
		{
			constexpr std::uintptr_t DriverSeat{ 0x23E4 };				// float (1).
			constexpr std::uintptr_t LeftDriverHijackerSeat{ 0x23F0 };	// float (1).
			constexpr std::uintptr_t RightDriverHijackerSeat{ 0x23FC };	// float (1).
			constexpr std::uintptr_t BackHijackerSeat{ 0x2408 };		// float (1).

			namespace PlasmaTurret
			{
				constexpr std::uintptr_t GunnerSeat{ 0x1EC4 };	// float (1).
				// TODO: Get IsFiring.
			}
		}
	}

	namespace Weapon
	{
		namespace Flag
		{
			constexpr std::uintptr_t Team{ 0x6D1 };	// uint8.
		}

		namespace Bomb
		{
			constexpr std::uintptr_t Team{ 0x6E5 };	// uint8.
		}
	}

	namespace Crate
	{
		// Just for Flag & Capture Plate.
		namespace ObjectiveSpawn
		{
			constexpr std::uintptr_t Radius{ 0x258 };		// float (1).
			constexpr std::uintptr_t Length{ 0x25C };		// float (1).
			constexpr std::uintptr_t Top{ 0x260 };			// float (1).
			constexpr std::uintptr_t Bottom{ 0x264 };		// float (1).
			constexpr std::uintptr_t ShapeKind{ 0x268 };	// uint8.
			constexpr std::uintptr_t Team{ 0x271 };			// uint8.
		}

		namespace HillMarker
		{
			constexpr std::uintptr_t Radius{ 0x274 };		// float (1).
			constexpr std::uintptr_t Length{ 0x278 };		// float (1).
			constexpr std::uintptr_t Top{ 0x27C };			// float (1).
			constexpr std::uintptr_t Bottom{ 0x280 };		// float (1).
			constexpr std::uintptr_t ShapeKind{ 0x284 };	// uint8.
			constexpr std::uintptr_t Team{ 0x28D };			// uint8.
		}

		namespace Teleport
		{
			constexpr std::uintptr_t Radius{ 0x290 };			// float (1).
			constexpr std::uintptr_t Length{ 0x294 };			// float (1).
			constexpr std::uintptr_t Top{ 0x298 };				// float (1).
			constexpr std::uintptr_t Bottom{ 0x29C };			// float (1).
			constexpr std::uintptr_t ShapeKind{ 0x2A0 };		// uint8.
			constexpr std::uintptr_t Channel{ 0x2A5 };			// uint8.
			constexpr std::uintptr_t Allowed{ 0x2A6 };			// uint8.
		}
	}

    namespace Scenery
    {
        // Initial & Respawn.
        namespace SpawnPoint
        {
            constexpr std::uintptr_t Team{ 0x2C1 };	// uint8.
        }

        // Safe, SoftSafe, Kill & SoftKill.
        namespace Boundary
        {
            constexpr std::uintptr_t Team{ 0x2A5 };			// uint8.
            constexpr std::uintptr_t Radius{ 0x28C };		// float (1).
            constexpr std::uintptr_t Length{ 0x290 };		// float (1).
            constexpr std::uintptr_t Top{ 0x294 };			// float (1).
            constexpr std::uintptr_t Bottom{ 0x298 };		// float (1).
            constexpr std::uintptr_t ShapeKind{ 0x29C };	// uint8.
        }
    }
}