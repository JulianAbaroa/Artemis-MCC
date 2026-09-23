export module Tables.Object.Type:Vehicle;

import :Offset;
import Common.Math.Type;
import Common.Tag.Name;
import Resolved.Stats.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using SeatType = Resolved::Stats::Type::Vehi::SeatType;
}

export namespace Tables::Object::Type::Vehicle
{
	enum class Kind : std::uint8_t
	{
		Unknown,
		Banshee,
		Falcon, FalconSensor, FalconTurret, FalconGrenadeTurret, FalconChinGun,
		Ghost,
		Mongoose,
		Revenant, RevenantPlasmaTurret,
		Scorpion, ScorpionTurret, ScorpionCannon,
		ShadeTurret, ShadePlasmaCannon, ShadeFlakCannon,
		Warthog, WarthogChaingun, WarthogGauss, WarthogRocket, WarthogTroop,
		Wraith, WraithPlasmaTurret, WraithCannon,
		Sabre,
		Seraph,
		CartElectric,
		Forklift,
		Pickup,
		TruckCab,
		OniVan,
	};

	namespace Seat
	{
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
		struct Seat
		{
			std::uintptr_t Offset{};
			const char* Name{};
			bool IsHijackable{};
			bool IsHijackerSlot{};
		};

		struct Marker
		{
			std::uint32_t SeatMarkerNameId{};
			std::int8_t NodeIndex{};
			Vec3 LocalTranslation{};
			Vec3 LocalRotation{};
			SeatType SeatType{};
			float EntryRadius{};
		};

		struct Layout
		{
			Kind Kind{};
			std::vector<Seat> Seats{};
			std::vector<Marker> SeatMarkers{};
		};
	}

	namespace Offset = Tables::Object::Type::Offset;
	namespace Banshee = Offset::Vehicle::Banshee;
	namespace Falcon = Offset::Vehicle::Falcon;
	namespace Ghost = Offset::Vehicle::Ghost;
	namespace Mongoose = Offset::Vehicle::Mongoose;
	namespace Revenant = Offset::Vehicle::Revenant;
	namespace Scorpion = Offset::Vehicle::Scorpion;
	namespace ShadeTurret =	Offset::Vehicle::ShadeTurret;
	namespace Warthog =	Offset::Vehicle::Warthog;
	namespace Wraith = Offset::Vehicle::Wraith;

	// TODO: Add the rest of the vehicles.
	inline const std::vector<Seat::Layout> Layouts =
	{
		{ 
			Kind::Banshee,
			{
				// Seat.
				{ Banshee::DriverSeat,					"Driver",				true, false },

				// Hijackers.
				{ Banshee::DriverRightHijacker,			"RightHijacker",		false, true },
				{ Banshee::DriverLeftHijacker,			"LeftHijacker",			false, true },
			}
		},
		{ 
			Kind::Falcon,
			{
				// Seats.
				{ Falcon::DriverSeat,					"Driver",				true, false },
				{ Falcon::FrontRightPassengerSeat,		"FrontRightPassenger",	false, false },
				{ Falcon::FrontLeftPassengerSeat,		"FrontLeftPassenger",	false, false },
				{ Falcon::BackRightPassengerSeat,		"BackRightPassenger",	false, false },
				{ Falcon::BackLeftPassengerSeat,		"BackLeftPassenger",	false, false },
				{ Falcon::BackMiddlePassengerSeat,		"BackMiddlePassenger",	false, false },

				// Hijackers.
				{ Falcon::RightDriverHijacker,			"DriverRightHijacker",	false, true },
				{ Falcon::LeftDriverHijacker,			"DriverLeftHijacker",	false, true },
			}
		},
		{ 
			Kind::FalconTurret,
			{
				// Gunners.
				{ Falcon::Turret::GunnerSeat,			"Gunner",				true, false },

				// Hijackers.
				{ Falcon::Turret::GunnerHijacker,		"GunnerHijacker",		false, true },
			}
		},
		{ 
			Kind::FalconGrenadeTurret,
			{
				// Gunners.
				{ Falcon::Grenadier::GrenadierSeat,		"Gunner",				true, false },

				// Hijackers.
				{ Falcon::Grenadier::GrenadierHijacker, "GunnerHijacker",		false, true },
			}
		},
		{ 
			Kind::Ghost,
			{
				// Seat.
				{ Ghost::DriverSeat,					"Driver",				true, false },

				// Hijackers.
				{ Ghost::FrontRightDriverHijacker,		"FrontRightHijacker",	false, true },
				{ Ghost::FrontLeftDriverHijacker,		"FrontLeftHijacker",	false, true },
				{ Ghost::BackDriverHijacker,			"BackHijacker",			false, true },
			}
		},
		{ 
			Kind::Mongoose,
			{
				// Seats.
				{ Mongoose::DriverSeat,					"Driver",				true, false },
				{ Mongoose::PassengerSeat,				"Passenger",			false, false },

				// Hijackers.
				{ Mongoose::DriverHijacker,				"DriverHijacker",		false, true },
				{ Mongoose::PassengerHijacker,			"PassengerHijacker",	false, true },
			}
		},
		{ 
			Kind::Revenant,
			{
				// Seats.
				{ Revenant::DriverSeat,					"Driver",				true, false },
				{ Revenant::PassengerSeat,				"Passenger",			true, false },

				// Hijackers.
				{ Revenant::DriverHijacker,				"DriverHijacker",		false, true },
				{ Revenant::PassengerHijacker,			"PassengerHijacker",	false, true },
			}
		},
		{ 
			Kind::Scorpion,
			{
				// Seat.
				{ Scorpion::DriverSeat,					"Driver",				true, false },

				// Hijackers.
				{ Scorpion::RightDriverHijacker,		"RightDriverHijacker",	false, true },
				{ Scorpion::LeftDriverHijacker,			"LeftDriverHijacker",	false, true },
				{ Scorpion::BackHijacker,				"BackHijacker",			false, true },
			}
		},
		{ 
			Kind::ScorpionTurret,
			{
				// Gunner.
				{ Scorpion::Turret::GunnerSeat,			"Gunner",				false, false },
			}
		},
		{ 
			Kind::ShadeTurret,
			{
				// Seat.
				{ ShadeTurret::GunnerSeat,				"Gunner",				false, false },
			}
		},
		{ 
			Kind::Warthog,
			{
				// Seats.
				{ Warthog::DriverSeat,					"Driver",				true, false },
				{ Warthog::PassengerSeat,				"Passenger",			true, false },

				// Hijackers.
				{ Warthog::DriverHijacker,				"DriverHijacker",		false, true },
				{ Warthog::PassengerHijacker,			"PassengerHijacker",	false, true },
			}
		},
		{ 
			Kind::WarthogChaingun,
			{
				// Gunner.
				{ Warthog::Turret::GunnerSeat,			"Gunner",				true, false },

				// Hijacker.
				{ Warthog::Turret::GunnerHijackerSeat, "GunnerHijacker",		false, true },
			}
		},
		{ 
			Kind::WarthogGauss,
			{
				// Gunner.
				{ Warthog::Gauss::GunnerSeat,			"Gunner",				true, false },

				// Hijacker.
				{ Warthog::Gauss::GunnerHijackerSeat,	"GunnerHijacker",		false, true },
			}
		},
		{ 
			Kind::WarthogRocket,
			{
				// Gunner.
				{ Warthog::Rocket::GunnerSeat,			"Gunner",				true, false },

				// Hijacker.
				{ Warthog::Rocket::GunnerHijackerSeat,	"GunnerHijacker",		false, true },
			}
		},
		{ 
			Kind::Wraith,
			{
				// Seat.
				{ Wraith::DriverSeat,					"Driver",				true, false },

				// Hijackers.
				{ Wraith::RightDriverHijackerSeat,		"RightDriverHijacker",	false, true },
				{ Wraith::LeftDriverHijackerSeat,		"LeftDriverHijacker",	false, true },
				{ Wraith::BackHijackerSeat,				"BackHijacker",			false, true },
			}
		},
		{ 
			Kind::WraithPlasmaTurret,
			{
				// Gunner.
				{ Wraith::PlasmaTurret::GunnerSeat,		"Gunner",				false, false },
			}
		},
	};

	// Returns the vehicle type based on the object's tag name.
	// Hardcoded solution, but those tag names are not going to change.
	inline Kind ResolveVehicleType(const std::string& tagName)
	{
		namespace Vehicle = Common::Tag::Name::Vehicle;
	
		static const std::unordered_map<std::string, Kind> map =
		{
			{ Vehicle::Banshee,				Kind::Banshee				},
			{ Vehicle::Falcon,				Kind::Falcon				},
			{ Vehicle::FalconSensor,		Kind::FalconGrenadeTurret	},
			{ Vehicle::FalconTurretRight,   Kind::FalconTurret			},
			{ Vehicle::FalconTurretLeft,    Kind::FalconTurret			},
			{ Vehicle::FalconGrenadeRight,	Kind::FalconGrenadeTurret	},
			{ Vehicle::FalconGrenadeLeft,	Kind::FalconGrenadeTurret	},
			{ Vehicle::FalconChinGun,		Kind::FalconGrenadeTurret	},
			{ Vehicle::Ghost,				Kind::Ghost					},
			{ Vehicle::Mongoose,			Kind::Mongoose				},
			{ Vehicle::Revenant,			Kind::Revenant				},
			{ Vehicle::RevenantPlasmaTurret,Kind::Revenant				},
			{ Vehicle::Scorpion,			Kind::Scorpion				},
			{ Vehicle::ScorpionTurret,      Kind::ScorpionTurret		},
			{ Vehicle::ScorpionCannon,      Kind::ScorpionCannon		},
			{ Vehicle::Shade,				Kind::ShadeTurret			},
			{ Vehicle::ShadePlasmaCannon,	Kind::ShadePlasmaCannon		},
			{ Vehicle::ShadeFlakCannon,		Kind::ShadeFlakCannon		},
			{ Vehicle::Warthog,				Kind::Warthog				},
			{ Vehicle::WarthogChaingun,		Kind::WarthogChaingun		},
			{ Vehicle::WarthogGauss,		Kind::WarthogGauss			},
			{ Vehicle::WarthogRocket,		Kind::WarthogRocket			},
			{ Vehicle::WarthogTroop,		Kind::WarthogTroop			},
			{ Vehicle::Wraith,				Kind::Wraith				},
			{ Vehicle::WraithPlasmaTurret,	Kind::WraithPlasmaTurret	},
			{ Vehicle::WraithCannon,		Kind::WraithCannon			},
			{ Vehicle::Sabre,				Kind::Sabre					},
			{ Vehicle::Seraph,				Kind::Seraph				},
			{ Vehicle::CartElectric,		Kind::CartElectric			},
			{ Vehicle::Forklift,			Kind::Forklift				},
			{ Vehicle::Pickup,				Kind::Pickup				},
			{ Vehicle::TruckCab,			Kind::TruckCab				},
			{ Vehicle::OniVan,				Kind::OniVan				},
		};
	
		auto it = map.find(tagName);
		return it != map.end() ? it->second : Kind::Unknown;
	}

	inline const Seat::Layout* GetSeatLayout(Kind kind)
	{
		for (const auto& layout : Layouts)
		{
			if (layout.Kind == kind) return &layout;
		}
		return nullptr;
	}

	struct Vehicle
	{
		std::uintptr_t Base{};
		Kind Kind{};
		const Seat::Layout* SeatLayout{};

		float BoostThrottle{};
		float BoostEnergy{};
		float BoostCooldown{};

		bool IsSeatFree(const Seat::Seat& seat) const
		{
			return *reinterpret_cast<const float*>(Base + seat.Offset) == 1.0f;
		}
	};
}