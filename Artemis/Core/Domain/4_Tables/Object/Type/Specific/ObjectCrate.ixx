export module Tables.Object.Type:Crate;

import Common.Tag.Type;
import Common.ZoneShape.Type;
import std;

namespace
{
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
		};

		auto it = map.find(tagName);
		return it != map.end() ? it->second : Kind::Unknown;
	}

	struct Crate
	{
		std::uintptr_t Base{};
		Kind Kind{};

		std::optional<Teleport::Teleport> Teleport{};
	};
}