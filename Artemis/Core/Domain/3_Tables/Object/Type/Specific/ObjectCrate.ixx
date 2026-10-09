export module Tables.Object.Type:Crate;

import :Offset;
import Common.Tag.Type;
import Common.Team.Type;
import Common.ZoneShape.Type;
import std;

namespace
{
	using Team = Common::Team::Type::Team;
	using ZoneShape = Common::ZoneShape::Type::ZoneShape;
}

export namespace Tables::Object::Type::Crate
{
	enum class Kind : std::uint8_t
	{
		Unknown = 0,

		// Zone / Objective.
		FlagStand, CapturePlate, HillMarker,
		TeleportSender, TeleportReceiver, TeleportTwoWay,

		// Destructible.
		FusionCoil, Landmine, PlasmaBattery,
		PropaneTank, Pallete,

		Lift, Shield,
	};

	namespace Teleport
	{
		enum class Allowed : std::uint8_t
		{
			None = 0x00,
			NoPlayers = 0x01,
			AllowGround = 0x02,
			AllowHeavy = 0x04,
			AllowFlying = 0x08,
			AllowProjctile = 0x10,
		};

		inline Allowed operator|(Allowed a, Allowed b)
		{
			return static_cast<Allowed>(
				static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
		}

		inline bool HasFlag(Allowed mask, Allowed flag)
		{
			return (static_cast<std::uint8_t>(mask) &
				static_cast<std::uint8_t>(flag)) != 0;
		}

		struct Teleport
		{
			ZoneShape ZoneShape{};
			std::uint8_t Channel{};
			Allowed Allowed{};
		};
	}

	namespace Lift
	{
		enum class Angle : std::uint8_t
		{
			Curved, Vertical, Redirected,
		};

		enum class Force : std::uint8_t
		{
			Default, Light, Heavy, Vehicle,
		};

		struct Lift
		{
			Angle Angle{};
			Force Force{};
		};
	}

	namespace Shield
	{
		enum class Kind : std::uint8_t
		{
			OneWay, TwoWay, Blocker,
		};

		struct Shield
		{
			Kind Kind{};
			bool IsShieldDoor{};
		};
	}

	namespace Zone
	{
		struct Zone
		{
			ZoneShape Shape{};
			Team Team{};
		};
	}

	namespace Destructible
	{
		struct Destructible
		{
			float Health{};
			std::optional<std::uint16_t> RegenerationDelay{};
		};
	}

	inline Kind ResolveCrateType(const std::string& tagName)
	{
		namespace Name = Common::Tag::Type;

		static const std::unordered_map<std::string, Kind> map =
		{
			// Objectives.
			{ Name::Objective::k_FlagStand,				Kind::FlagStand			},
			{ Name::Objective::k_CapturePlate,			Kind::CapturePlate		},
			{ Name::Objective::k_HillMarker,			Kind::HillMarker		},

			// Teleports.
			{ Name::Teleport::k_TeleportSender,			Kind::TeleportSender	},
			{ Name::Teleport::k_TeleportReceiver,		Kind::TeleportReceiver	},
			{ Name::Teleport::k_TeleportTwoWay,			Kind::TeleportTwoWay	},

			// Palletes.
			{ Name::Pallets::k_Pallet,					Kind::Pallete,			},
			{ Name::Pallets::k_PalletLarge,				Kind::Pallete,			},

			// Explosives.
			{ Name::Explosive::k_FusionCoil,			Kind::FusionCoil		},
			{ Name::Explosive::k_Landmine,				Kind::Landmine			},
			{ Name::Explosive::k_PlasmaBattery,			Kind::PlasmaBattery		},
			{ Name::Explosive::k_PropaneTank,			Kind::PropaneTank		},

			// Lift.
			{ Name::Lift::k_ManCannon,					Kind::Lift				},
			{ Name::Lift::k_ManCannonHeavy,				Kind::Lift				},
			{ Name::Lift::k_ManCannonLight,				Kind::Lift				},
			{ Name::Lift::k_VehicleManCannon,			Kind::Lift				},
			{ Name::Lift::k_GravityLift,				Kind::Lift				},
			{ Name::Lift::k_ManCannonMCC,				Kind::Lift				},
			{ Name::Lift::k_ManCannonHeavyMCC,			Kind::Lift				},
			{ Name::Lift::k_ManCannonLightMCC,			Kind::Lift				},
			{ Name::Lift::k_ForerunnerGravityLift,		Kind::Lift				},
			{ Name::Lift::k_ForerunnerGravityLiftTall,	Kind::Lift				},
			{ Name::Lift::k_ManCannonHumanMCC,			Kind::Lift				},

			// Shield.
			{ Name::Shield::k_OneWayDoorSmall,			Kind::Shield			},
			{ Name::Shield::k_OneWayDoorMedium,			Kind::Shield			},
			{ Name::Shield::k_OneWayDoorLarge,			Kind::Shield			},
			{ Name::Shield::k_TwoWayDoorSmall,			Kind::Shield			},
			{ Name::Shield::k_TwoWayDoorMedium,			Kind::Shield			},
			{ Name::Shield::k_TwoWayDoorLarge,			Kind::Shield			},
			{ Name::Shield::k_OneWayDoorXSmall,			Kind::Shield			},
			{ Name::Shield::k_OneWayDoorGarage,			Kind::Shield			},
			{ Name::Shield::k_ShieldWallSmall,			Kind::Shield			},
			{ Name::Shield::k_ShieldWallMedium,			Kind::Shield			},
			{ Name::Shield::k_ShieldWallLarge,			Kind::Shield			},
			{ Name::Shield::k_ShieldWallXLarge,			Kind::Shield			},
			{ Name::Shield::k_ShieldDoorSmall,			Kind::Shield			},
			{ Name::Shield::k_ShieldDoorMedium,			Kind::Shield			},
			{ Name::Shield::k_ShieldDoorLarge,			Kind::Shield			},
			{ Name::Shield::k_HangarShieldDoorSmall,		Kind::Shield			},
			{ Name::Shield::k_HangarShieldDoorSmallSolid,	Kind::Shield			},
			{ Name::Shield::k_HangarShieldDoorLarge,		Kind::Shield			},
			{ Name::Shield::k_HangarShieldDoorLargeSolid,	Kind::Shield			},
			{ Name::Shield::k_DropShield,					Kind::Shield			},
		};

		auto it = map.find(tagName);
		return it != map.end() ? it->second : Kind::Unknown;
	}

	struct ZoneOffsets
	{
		std::uintptr_t Radius{};
		std::uintptr_t Length{};
		std::uintptr_t Top{};
		std::uintptr_t Bottom{};
		std::uintptr_t ZoneType{};
		std::uintptr_t Team{};
	};

	inline std::optional<ZoneOffsets> ResolveZoneOffsets(Kind kind)
	{
		namespace Offset = Tables::Object::Type::Offset;

		static const std::unordered_map<Kind, ZoneOffsets> map =
		{
			{
				Kind::FlagStand,
				{
					Offset::Crate::ObjectiveSpawn::Radius,
					Offset::Crate::ObjectiveSpawn::Length,
					Offset::Crate::ObjectiveSpawn::Top,
					Offset::Crate::ObjectiveSpawn::Bottom,
					Offset::Crate::ObjectiveSpawn::ShapeKind,
					Offset::Crate::ObjectiveSpawn::Team
				}
			},
			{
				Kind::CapturePlate,
				{
					Offset::Crate::ObjectiveSpawn::Radius,
					Offset::Crate::ObjectiveSpawn::Length,
					Offset::Crate::ObjectiveSpawn::Top,
					Offset::Crate::ObjectiveSpawn::Bottom,
					Offset::Crate::ObjectiveSpawn::ShapeKind,
					Offset::Crate::ObjectiveSpawn::Team
				}
			},
			{
				Kind::HillMarker,
				{
					Offset::Crate::HillMarker::Radius,
					Offset::Crate::HillMarker::Length,
					Offset::Crate::HillMarker::Top,
					Offset::Crate::HillMarker::Bottom,
					Offset::Crate::HillMarker::ShapeKind,
					Offset::Crate::HillMarker::Team
				}
			},
		};

		auto it = map.find(kind);
		if (it == map.end()) return std::nullopt;
			return it->second;
	}

	struct Crate
	{
		std::uintptr_t Base{};
		Kind Kind{};

		std::optional<Zone::Zone> Zone{};
		std::optional<Teleport::Teleport> Teleport{};
		std::optional<Destructible::Destructible> Destructible{};
		std::optional<Lift::Lift> Lift{};
		std::optional<Shield::Shield> Shield{};
	};
}