#pragma once

#include "Core/Types/TagName.h"
#include "../ObjectOffsets.h"
#include <unordered_map>
#include <vector>

enum class VehicleType : uint8_t
{
	Unknown,
	Banshee,
	Falcon,
	FalconSensor,
	FalconTurret,
	FalconGrenadeTurret,
	FalconChinGun,
	Ghost,
	Mongoose,
	Revenant,
	RevenantPlasmaTurret,
	Scorpion,
	ScorpionTurret,
	ScorpionCannon,
	ShadeTurret,
	ShadePlasmaCannon,
	ShadeFlakCannon,
	Warthog,
	WarthogChaingun,
	WarthogGauss,
	WarthogRocket,
	WarthogTroop,
	Wraith,
	WraithPlasmaTurret,
	WraithCannon,
	Sabre,
	Seraph,
	CartElectric,
	Forklift,
	Pickup,
	TruckCab,
	OniVan,
};

// Inside the vehicle object in game's memory, there are specific offsets which
// holds information about the state of the seats. I haven't confirmed which specific
// state holds, probably related to animations and-or more things.
//
// Each seat of the vehicle has a pair of floats and one static flag '0x30'.
// Both floats are 1.0f when the seat is free, and 0.0f when the seat is occupied.
// And since the seats are written one after another, we can know exactly which
// seat is occupied at any given moment.
//
// Not only that, but there are the 'hijacker' seats, which are written after the
// normal seats, following the same structure and behaviour. So, we can know whenever
// a specific seat is being hijacked by another player. We don't know specificly
// which player, but with other data we get from other sides of this project, we
// might be able to do this connection.
struct SeatInfo
{
	uintptr_t Offset;
	const char* Name;
	bool IsHijackable;
	bool IsHijackerSlot;
};

struct VehicleSeatLayout
{
	VehicleType type;
	std::vector<SeatInfo> seats;
};

struct VehicleBaseOffsets
{
	uintptr_t Health;
	uintptr_t RegenerationDelay;
};

namespace Vehicle = ObjectOffsets::Vehicle;
namespace Banshee = Vehicle::Banshee;
namespace Falcon = Vehicle::Falcon;
namespace Ghost = Vehicle::Ghost;
namespace Mongoose = Vehicle::Mongoose;
namespace Revenant = Vehicle::Revenant;
namespace Scorpion = Vehicle::Scorpion;
namespace ShadeTurret = Vehicle::ShadeTurret;
namespace Warthog = Vehicle::Warthog;
namespace Wraith = Vehicle::Wraith;

// TODO: Add the rest of the vehicles.
inline const std::vector<VehicleSeatLayout> VehicleLayouts =
{
	{ VehicleType::Banshee, 
		{ 
			// Seat.
			{ Banshee::DriverSeat, "Driver", true, false }, 

			// Hijackers.
			{ Banshee::DriverRightHijacker, "RightHijacker", false, true },
			{ Banshee::DriverLeftHijacker, "LeftHijacker", false, true },
		} 
	},
	{ VehicleType::Falcon,
		{
			// Seats.
			{ Falcon::DriverSeat, "Driver", true, false },
			{ Falcon::FrontRightPassengerSeat, "FrontRightPassenger", false, false },
			{ Falcon::FrontLeftPassengerSeat, "FrontLeftPassenger", false, false },
			{ Falcon::BackRightPassengerSeat, "BackRightPassenger", false, false },
			{ Falcon::BackLeftPassengerSeat, "BackLeftPassenger", false, false },
			{ Falcon::BackMiddlePassengerSeat, "BackMiddlePassenger", false, false },

			// Hijackers.
			{ Falcon::RightDriverHijacker, "DriverRightHijacker", false, true },
			{ Falcon::LeftDriverHijacker, "DriverLeftHijacker", false, true },
		}
	},
	{ VehicleType::FalconTurret,
		{
			// Gunners.
			{ Falcon::Turret::GunnerSeat, "Gunner", true, false },

			// Hijackers.
			{ Falcon::Turret::GunnerHijacker, "GunnerHijacker", false, true },
		}
	},
	{ VehicleType::FalconGrenadeTurret,
		{
			// Gunners.
			{ Falcon::Grenadier::GrenadierSeat, "Gunner", true, false },

			// Hijackers.
			{ Falcon::Grenadier::GrenadierHijacker, "GunnerHijacker", false, true },
		}
	},
	{ VehicleType::Ghost,
		{
			// Seat.
			{ Ghost::DriverSeat, "Driver", true, false },

			// Hijackers.
			{ Ghost::FrontRightDriverHijacker, "FrontRightHijacker", false, true },
			{ Ghost::FrontLeftDriverHijacker, "FrontLeftHijacker", false, true },
			{ Ghost::BackDriverHijacker, "BackHijacker", false, true },
		}
	},
	{ VehicleType::Mongoose,
		{
			// Seats.
			{ Mongoose::DriverSeat, "Driver", true, false },
			{ Mongoose::PassengerSeat, "Passenger", false, false },

			// Hijackers.
			{ Mongoose::DriverHijacker, "DriverHijacker", false, true },
			{ Mongoose::PassengerHijacker, "PassengerHijacker", false, true },
		}
	},
	{ VehicleType::Revenant,
		{
			// Seats.
			{ Revenant::DriverSeat, "Driver", true, false },
			{ Revenant::PassengerSeat, "Passenger", true, false },

			// Hijackers.
			{ Revenant::DriverHijacker, "DriverHijacker", false, true },
			{ Revenant::PassengerHijacker, "PassengerHijacker", false, true },
		}
	},
	{ VehicleType::Scorpion,
		{
			// Seat.
			{ Scorpion::DriverSeat, "Driver", true, false },

			// Hijackers.
			{ Scorpion::RightDriverHijacker, "RightDriverHijacker", false, true },
			{ Scorpion::LeftDriverHijacker, "LeftDriverHijacker", false, true },
			{ Scorpion::BackHijacker, "BackHijacker", false, true },
		}
	},
	{ VehicleType::ScorpionTurret,
		{
			// Gunner.
			{ Scorpion::Turret::GunnerSeat, "Gunner", false, false },
		}
	},
	{ VehicleType::ShadeTurret,
		{
			// Seat.
			{ ShadeTurret::GunnerSeat, "Gunner", false, false },
		}
	},
	{ VehicleType::Warthog,
		{
			// Seats.
			{ Warthog::DriverSeat, "Driver", true, false },
			{ Warthog::PassengerSeat, "Passenger", true, false },

			// Hijackers.
			{ Warthog::DriverHijacker, "DriverHijacker", false, true },
			{ Warthog::PassengerHijacker, "PassengerHijacker", false, true },
		}
	},
	{ VehicleType::WarthogChaingun,
		{
			// Gunner.
			{ Warthog::Turret::GunnerSeat, "Gunner", true, false },

			// Hijacker.
			{ Warthog::Turret::GunnerHijackerSeat, "GunnerHijacker", false, true },
		}
	},
	{ VehicleType::WarthogGauss,
		{
			// Gunner.
			{ Warthog::Gauss::GunnerSeat, "Gunner", true, false },

			// Hijacker.
			{ Warthog::Gauss::GunnerHijackerSeat, "GunnerHijacker", false, true },
		}
	},
	{ VehicleType::WarthogRocket,
		{
			// Gunner.
			{ Warthog::Rocket::GunnerSeat, "Gunner", true, false },

			// Hijacker.
			{ Warthog::Rocket::GunnerHijackerSeat, "GunnerHijacker", false, true },
		}
	},
	{ VehicleType::Wraith,
		{
			// Seat.
			{ Wraith::DriverSeat, "Driver", true, false },

			// Hijackers.
			{ Wraith::RightDriverHijackerSeat, "RightDriverHijacker", false, true },
			{ Wraith::LeftDriverHijackerSeat, "LeftDriverHijacker", false, true },
			{ Wraith::BackHijackerSeat, "BackHijacker", false, true },
		}
	},
	{ VehicleType::WraithPlasmaTurret,
		{
			// Gunner.
			{ Wraith::PlasmaTurret::GunnerSeat, "Gunner", false, false },
		}
	},
};

namespace VehicleObjectHelpers
{
	// Returns the vehicle type based on the object's tag name.
	// Hardcoded solution, but those tag names are not going to change.
	inline VehicleType ResolveVehicleType(const std::string& tagName)
	{
		namespace Vehicle = TagName::Vehicle;

		static const std::unordered_map<std::string, VehicleType> map =
		{
			{ Vehicle::Banshee,				VehicleType::Banshee				},
			{ Vehicle::Falcon,				VehicleType::Falcon					},
			{ Vehicle::FalconSensor,		VehicleType::FalconGrenadeTurret	},
			{ Vehicle::FalconTurretRight,   VehicleType::FalconTurret			},
			{ Vehicle::FalconTurretLeft,    VehicleType::FalconTurret			},
			{ Vehicle::FalconGrenadeRight,	VehicleType::FalconGrenadeTurret	},
			{ Vehicle::FalconGrenadeLeft,	VehicleType::FalconGrenadeTurret	},
			{ Vehicle::FalconChinGun,		VehicleType::FalconGrenadeTurret	},
			{ Vehicle::Ghost,				VehicleType::Ghost					},
			{ Vehicle::Mongoose,			VehicleType::Mongoose				},
			{ Vehicle::Revenant,			VehicleType::Revenant				},
			{ Vehicle::RevenantPlasmaTurret,VehicleType::Revenant				},
			{ Vehicle::Scorpion,			VehicleType::Scorpion				},
			{ Vehicle::ScorpionTurret,      VehicleType::ScorpionTurret			},
			{ Vehicle::ScorpionCannon,      VehicleType::ScorpionCannon			},
			{ Vehicle::Shade,				VehicleType::ShadeTurret					},
			{ Vehicle::ShadePlasmaCannon,	VehicleType::ShadePlasmaCannon		},
			{ Vehicle::ShadeFlakCannon,		VehicleType::ShadeFlakCannon		},
			{ Vehicle::Warthog,				VehicleType::Warthog				},
			{ Vehicle::WarthogChaingun,		VehicleType::WarthogChaingun		},
			{ Vehicle::WarthogGauss,		VehicleType::WarthogGauss			},
			{ Vehicle::WarthogRocket,		VehicleType::WarthogRocket			},
			{ Vehicle::WarthogTroop,		VehicleType::WarthogTroop			},
			{ Vehicle::Wraith,				VehicleType::Wraith					},
			{ Vehicle::WraithPlasmaTurret,	VehicleType::WraithPlasmaTurret		},
			{ Vehicle::WraithCannon,		VehicleType::WraithCannon			},
			{ Vehicle::Sabre,				VehicleType::Sabre					},
			{ Vehicle::Seraph,				VehicleType::Seraph					},
			{ Vehicle::CartElectric,		VehicleType::CartElectric			},
			{ Vehicle::Forklift,			VehicleType::Forklift				},
			{ Vehicle::Pickup,				VehicleType::Pickup					},
			{ Vehicle::TruckCab,			VehicleType::TruckCab				},
			{ Vehicle::OniVan,				VehicleType::OniVan					},
		};

		auto it = map.find(tagName);
		return it != map.end() ? it->second : VehicleType::Unknown;
	}

	inline const VehicleSeatLayout* GetSeatLayout(VehicleType type)
	{
		for (const auto& layout : VehicleLayouts)
		{
			if (layout.type == type) return &layout;
		}
		return nullptr;
	}
}

struct VehicleObject
{
	uintptr_t Base;
	VehicleType Type;
	const VehicleSeatLayout* SeatLayout;

	float BoostThrottle;
	float BoostEnergy;
	float BoostCooldown;

	bool IsSeatFree(const SeatInfo& seatInfo) const
	{
		return *reinterpret_cast<const float*>(Base + seatInfo.Offset) == 1.0f;
	}
};