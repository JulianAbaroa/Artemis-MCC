export module Tables.Object.Type:Vehicle;

import Common.Math.Type;
import Common.Tag.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using SeatKind = Resolved::Definitions::Type::Vehi::SeatKind;
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
		// The vehicle datum has a variable block (descriptor 0x560, see Datum.Object.Type:Block) with
		// one element of 12 bytes per seat of the vehicle tag: a pair of floats and one static flag '3'.
		// The first float is a smoothed value that ramps towards the second one (the target) every tick.
		// The target is 1.0f when the seat is free and 0.0f when the seat is occupied.
		// The elements follow the order of the seats in the tag, so a seat is found by its index.
		//
		// The 'hijacker' seats are written after the normal seats, following the same structure and
		// behaviour. So, we can know whenever a specific seat is being hijacked by another player.
		// We don't know specificly which player, but with other data we get from other sides of this
		// project, we might be able to do this connection.
		//
		// note: The index of each seat was derived from the offsets that were used before the block was
		// found. The Grenadier of the Falcon had inconsistent offsets, its indices are assumed.
		struct Seat
		{
			std::size_t Index{};
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
			SeatKind SeatKind{};
			float EntryRadius{};
		};

		struct Layout
		{
			Kind Kind{};
			std::vector<Seat> Seats{};
			std::vector<Marker> SeatMarkers{};
		};
	}

	// TODO: Add the rest of the vehicles.
	inline const std::vector<Seat::Layout> Layouts =
	{
		{
			Kind::Banshee,
			{
				// Seat.
				{ 0, "Driver",                true, false },

				// Hijackers.
				{ 1, "RightHijacker",         false, true },
				{ 2, "LeftHijacker",          false, true },
			}
		},
		{
			Kind::Falcon,
			{
				// Seats.
				{ 0, "Driver",                true, false },
				{ 2, "FrontRightPassenger",   false, false },
				{ 1, "FrontLeftPassenger",    false, false },
				{ 4, "BackRightPassenger",    false, false },
				{ 3, "BackLeftPassenger",     false, false },
				{ 5, "BackMiddlePassenger",   false, false },

				// Hijackers.
				{ 7, "DriverRightHijacker",   false, true },
				{ 6, "DriverLeftHijacker",    false, true },
			}
		},
		{
			Kind::FalconTurret,
			{
				// Gunners.
				{ 0, "Gunner",                true, false },

				// Hijackers.
				{ 1, "GunnerHijacker",        false, true },
			}
		},
		{
			Kind::FalconGrenadeTurret,
			{
				// Gunners.
				{ 0, "Gunner",                true, false },

				// Hijackers.
				{ 1, "GunnerHijacker",        false, true },
			}
		},
		{
			Kind::Ghost,
			{
				// Seat.
				{ 0, "Driver",                true, false },

				// Hijackers.
				{ 2, "FrontRightHijacker",    false, true },
				{ 3, "FrontLeftHijacker",     false, true },
				{ 1, "BackHijacker",          false, true },
			}
		},
		{
			Kind::Mongoose,
			{
				// Seats.
				{ 0, "Driver",                true, false },
				{ 1, "Passenger",             false, false },

				// Hijackers.
				{ 2, "DriverHijacker",        false, true },
				{ 3, "PassengerHijacker",     false, true },
			}
		},
		{
			Kind::Revenant,
			{
				// Seats.
				{ 0, "Driver",                true, false },
				{ 1, "Passenger",             true, false },

				// Hijackers.
				{ 2, "DriverHijacker",        false, true },
				{ 3, "PassengerHijacker",     false, true },
			}
		},
		{
			Kind::Scorpion,
			{
				// Seat.
				{ 0, "Driver",                true, false },

				// Hijackers.
				{ 5, "RightDriverHijacker",   false, true },
				{ 6, "LeftDriverHijacker",    false, true },
				{ 7, "BackHijacker",          false, true },
			}
		},
		{
			Kind::ScorpionTurret,
			{
				// Gunner.
				{ 0, "Gunner",                false, false },
			}
		},
		{
			Kind::ShadeTurret,
			{
				// Seat.
				{ 0, "Gunner",                false, false },
			}
		},
		{
			Kind::Warthog,
			{
				// Seats.
				{ 0, "Driver",                true, false },
				{ 1, "Passenger",             true, false },

				// Hijackers.
				{ 2, "DriverHijacker",        false, true },
				{ 3, "PassengerHijacker",     false, true },
			}
		},
		{
			Kind::WarthogChaingun,
			{
				// Gunner.
				{ 0, "Gunner",                true, false },

				// Hijacker.
				{ 1, "GunnerHijacker",        false, true },
			}
		},
		{
			Kind::WarthogGauss,
			{
				// Gunner.
				{ 0, "Gunner",                true, false },

				// Hijacker.
				{ 1, "GunnerHijacker",        false, true },
			}
		},
		{
			Kind::WarthogRocket,
			{
				// Gunner.
				{ 0, "Gunner",                true, false },

				// Hijacker.
				{ 1, "GunnerHijacker",        false, true },
			}
		},
		{
			Kind::Wraith,
			{
				// Seat.
				{ 0, "Driver",                true, false },

				// Hijackers.
				{ 2, "RightDriverHijacker",   false, true },
				{ 1, "LeftDriverHijacker",    false, true },
				{ 3, "BackHijacker",          false, true },
			}
		},
		{
			Kind::WraithPlasmaTurret,
			{
				// Gunner.
				{ 0, "Gunner",                false, false },
			}
		},
	};

	// Returns the vehicle type based on the object's tag name.
	// Hardcoded solution, but those tag names are not going to change.
	inline Kind ResolveVehicleType(const std::string& tagName)
	{
		namespace Vehicle = Common::Tag::Type::Vehicle;

		static const std::unordered_map<std::string, Kind> map =
		{
			{ Vehicle::k_Banshee,				Kind::Banshee				},
			{ Vehicle::k_Falcon,				Kind::Falcon				},
			{ Vehicle::k_FalconSensor,		Kind::FalconGrenadeTurret	},
			{ Vehicle::k_FalconTurretRight,   Kind::FalconTurret			},
			{ Vehicle::k_FalconTurretLeft,    Kind::FalconTurret			},
			{ Vehicle::k_FalconGrenadeRight,	Kind::FalconGrenadeTurret	},
			{ Vehicle::k_FalconGrenadeLeft,	Kind::FalconGrenadeTurret	},
			{ Vehicle::k_FalconChinGun,		Kind::FalconGrenadeTurret	},
			{ Vehicle::k_Ghost,				Kind::Ghost					},
			{ Vehicle::k_Mongoose,			Kind::Mongoose				},
			{ Vehicle::k_Revenant,			Kind::Revenant				},
			{ Vehicle::k_RevenantPlasmaTurret,Kind::Revenant				},
			{ Vehicle::k_Scorpion,			Kind::Scorpion				},
			{ Vehicle::k_ScorpionTurret,      Kind::ScorpionTurret		},
			{ Vehicle::k_ScorpionCannon,      Kind::ScorpionCannon		},
			{ Vehicle::k_Shade,				Kind::ShadeTurret			},
			{ Vehicle::k_ShadePlasmaCannon,	Kind::ShadePlasmaCannon		},
			{ Vehicle::k_ShadeFlakCannon,		Kind::ShadeFlakCannon		},
			{ Vehicle::k_Warthog,				Kind::Warthog				},
			{ Vehicle::k_WarthogChaingun,		Kind::WarthogChaingun		},
			{ Vehicle::k_WarthogGauss,		Kind::WarthogGauss			},
			{ Vehicle::k_WarthogRocket,		Kind::WarthogRocket			},
			{ Vehicle::k_WarthogTroop,		Kind::WarthogTroop			},
			{ Vehicle::k_Wraith,				Kind::Wraith				},
			{ Vehicle::k_WraithPlasmaTurret,	Kind::WraithPlasmaTurret	},
			{ Vehicle::k_WraithCannon,		Kind::WraithCannon			},
			{ Vehicle::k_Sabre,				Kind::Sabre					},
			{ Vehicle::k_Seraph,				Kind::Seraph				},
			{ Vehicle::k_CartElectric,		Kind::CartElectric			},
			{ Vehicle::k_Forklift,			Kind::Forklift				},
			{ Vehicle::k_Pickup,				Kind::Pickup				},
			{ Vehicle::k_TruckCab,			Kind::TruckCab				},
			{ Vehicle::k_OniVan,				Kind::OniVan				},
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

		// Target of each seat element, in the order of the tag. 1.0f = free, 0.0f = occupied.
		std::vector<float> SeatAvailability{};

		float BoostThrottle{};
		float BoostEnergy{};
		float BoostCooldown{};

		// A seat that the vehicle does not have is reported free.
		bool IsSeatFree(const Seat::Seat& seat) const
		{
			return seat.Index >= SeatAvailability.size() || SeatAvailability[seat.Index] == 1.0f;
		}
	};
}