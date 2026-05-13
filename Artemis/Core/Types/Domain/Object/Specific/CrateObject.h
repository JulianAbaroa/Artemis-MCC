#pragma once

#include "../../Team.h"
#include "../../TagName.h"
#include "../../ZoneShape.h"
#include "../ObjectOffsets.h"
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <string>

enum class CrateType : uint8_t
{
	Unknown = 0,

	// Zone / Objective.
	FlagStand,
	CapturePlate,
	HillMarker,
	TeleporterSender,
	TeleporterReceiver,
	TeleporterTwoWay,

	// Destructible.
	FusionCoil,
	Landmine,
	PlasmaBattery,
	PropaneTank,
	Pallete,

	// Lift.
	Lift,

	// Shield.
	Shield,
};

enum class AllowedObjects : uint8_t
{
	None = 0x00,
	NoPlayers = 0x01,
	AllowGround = 0x02,
	AllowHeavy = 0x04,
	AllowFlying = 0x08,
	AllowProjctile = 0x10,
};

inline AllowedObjects operator|(AllowedObjects a, AllowedObjects b)
{
	return static_cast<AllowedObjects>(
		static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool HasFlag(AllowedObjects mask, AllowedObjects flag)
{
	return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(flag)) != 0;
}

struct TeleporterData
{
	ZoneShape ZoneShape;
	uint8_t Channel;
	AllowedObjects AllowedObjects;
	Team Team;
};

struct ZoneData
{
	ZoneShape Shape;
	Team Team;
};

struct DestructibleData
{
	float Health;

	// Only FusionCoil has it.
	std::optional<uint16_t> RegenerationDelay;  
};

enum class AngleType : uint8_t
{
	Curved,
	Vertical,
	Redirected,
};

enum class ForceType : uint8_t
{
	Default,
	Light,
	Heavy,
	Vehicle,
};

struct LiftData
{
	AngleType AngleType;
	ForceType ForceType;
};

enum class ShieldType : uint8_t
{
	OneWay,
	TwoWay,
	Blocker,
};

struct ShieldData
{
	ShieldType ShieldType;
	bool IsShieldDoor;
};

namespace CrateObjectHelpers
{
	inline CrateType ResolveCrateType(const std::string& tagName)
	{
		using namespace TagName;

		static const std::unordered_map<std::string, CrateType> map =
		{
			// Objectives.
			{ Objective::FlagStand,				CrateType::FlagStand			},
			{ Objective::CapturePlate,			CrateType::CapturePlate			},
			{ Objective::HillMarker,			CrateType::HillMarker			},
			
			// Teleports.
			{ Teleport::TeleporterSender,		CrateType::TeleporterSender		},
			{ Teleport::TeleporterReceiver,		CrateType::TeleporterReceiver	},
			{ Teleport::TeleporterTwoWay,		CrateType::TeleporterTwoWay		},
			
			// Palletes.
			{ Palletes::Pallet,					CrateType::Pallete,				},
			{ Palletes::PalletLarge,			CrateType::Pallete,				},
			
			// Explosives.
			{ Explosive::FusionCoil,			CrateType::FusionCoil			},
			{ Explosive::Landmine,				CrateType::Landmine				},
			{ Explosive::PlasmaBattery,			CrateType::PlasmaBattery		},
			{ Explosive::PropaneTank,			CrateType::PropaneTank			},
			
			// Lift.
			{ Lift::ManCannon,					CrateType::Lift					},
			{ Lift::ManCannonHeavy,				CrateType::Lift					},
			{ Lift::ManCannonLight,				CrateType::Lift					},
			{ Lift::VehicleManCannon,			CrateType::Lift					},
			{ Lift::GravityLift,				CrateType::Lift					},
			{ Lift::ManCannonMCC,				CrateType::Lift					},
			{ Lift::ManCannonHeavyMCC,			CrateType::Lift					},
			{ Lift::ManCannonLightMCC,			CrateType::Lift					},
			{ Lift::ForerunnerGravityLift,		CrateType::Lift					},
			{ Lift::ForerunnerGravityLiftTall,	CrateType::Lift					},
			{ Lift::ManCannonHumanMCC,			CrateType::Lift					},

			// Shield.
			{ Shield::OneWayDoorSmall,				CrateType::Shield			},
			{ Shield::OneWayDoorMedium,				CrateType::Shield			},
			{ Shield::OneWayDoorLarge,				CrateType::Shield			},
			{ Shield::TwoWayDoorSmall,				CrateType::Shield			},
			{ Shield::TwoWayDoorMedium,				CrateType::Shield			},
			{ Shield::TwoWayDoorLarge,				CrateType::Shield			},
			{ Shield::OneWayDoorXSmall,				CrateType::Shield			},
			{ Shield::OneWayDoorGarage,				CrateType::Shield			},
			{ Shield::ShieldWallSmall,				CrateType::Shield			},
			{ Shield::ShieldWallMedium,				CrateType::Shield			},
			{ Shield::ShieldWallLarge,				CrateType::Shield			},
			{ Shield::ShieldWallXLarge,				CrateType::Shield			},
			{ Shield::ShieldDoorSmall,				CrateType::Shield			},
			{ Shield::ShieldDoorMedium,				CrateType::Shield			},
			{ Shield::ShieldDoorLarge,				CrateType::Shield			},
			{ Shield::HangarShieldDoorSmall,		CrateType::Shield			},
			{ Shield::HangarShieldDoorSmallSolid,	CrateType::Shield			},
			{ Shield::HangarShieldDoorLarge,		CrateType::Shield			},
			{ Shield::HangarShieldDoorLargeSolid,	CrateType::Shield			},
		};

		auto it = map.find(tagName);
		return it != map.end() ? it->second : CrateType::Unknown;
	}

	// Zone offsets shared by FlagStand, CapturePlate, HillMarker.
	struct ZoneOffsets
	{
		uintptr_t Radius;
		uintptr_t Length;
		uintptr_t Top;
		uintptr_t Bottom;
		uintptr_t ShapeType;
		uintptr_t Team;
	};

	inline std::optional<ZoneOffsets> ResolveZoneOffsets(CrateType type)
	{
		using namespace ObjectOffsets::Crate;

		static const std::unordered_map<CrateType, ZoneOffsets> map =
		{
			{ 
				CrateType::FlagStand,    
				{ 
					ObjectiveSpawn::Radius,    
					ObjectiveSpawn::Lenght,    
					ObjectiveSpawn::Top,    
					ObjectiveSpawn::Bottom,    
					ObjectiveSpawn::ShapeType,    
					ObjectiveSpawn::Team    
				} 
			},
			{ 
				CrateType::CapturePlate, 
				{ 
					ObjectiveSpawn::Radius,	
					ObjectiveSpawn::Lenght,		
					ObjectiveSpawn::Top, 
					ObjectiveSpawn::Bottom, 
					ObjectiveSpawn::ShapeType, 
					ObjectiveSpawn::Team 
				} 
			},
			{ 
				CrateType::HillMarker,   
				{ 
					HillMarker::Radius,   
					HillMarker::Lenght,   
					HillMarker::Top,   
					HillMarker::Bottom,   
					HillMarker::ShapeType,   
					HillMarker::Team   
				} 
			},
		};

		auto it = map.find(type);
		if (it == map.end()) return std::nullopt;
		return it->second;
	}

	struct DestructibleOffsets
	{
		uintptr_t Health{};
		std::optional<uintptr_t> RegenerationDelay{};
	};

	inline std::optional<DestructibleOffsets> ResolveDestructibleOffsets(CrateType type)
	{
		using namespace ObjectOffsets::Crate;

		static const std::unordered_map<CrateType, DestructibleOffsets> map =
		{
			{ CrateType::Pallete,		{ Palletes::Health,			std::nullopt					} },

			{ CrateType::FusionCoil,    { FusionCoil::Health,		FusionCoil::RegenerationDelay	} },
			{ CrateType::Landmine,      { Landmine::Health,			std::nullopt					} },
			{ CrateType::PlasmaBattery, { PlasmaBattery::Health,	std::nullopt					} },
			{ CrateType::PropaneTank,   { PropaneTank::Health,		std::nullopt					} },
		};

		auto it = map.find(type);
		if (it == map.end()) return std::nullopt;
		return it->second;
	}
}

struct CrateObject
{
	uintptr_t Base;
	CrateType Type;

	// Only one of these will be populated, based on Type.
	std::optional<ZoneData> Zone;					// FlagStand, CapturePlate, HillMarker.
	std::optional<TeleporterData> Teleporter;
	std::optional<DestructibleData> Destructible;	// FusionCoil, Landmine, PlasmaBattery, PropaneTank.
	std::optional<LiftData> Lift;
	std::optional<ShieldData> Shield;
};