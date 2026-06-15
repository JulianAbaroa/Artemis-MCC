#pragma once

#include "Core/Types/Team.h"
#include "Core/Types/TagName.h"
#include "Core/Types/ZoneShape.h"
#include "../ObjectOffsets.h"
#include <unordered_map>
#include <optional>

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

	Lift,
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
	return (static_cast<uint8_t>(mask) & 
		static_cast<uint8_t>(flag)) != 0;
}

struct TeleporterData
{
	ZoneShape ZoneShape;
	uint8_t Channel;
	AllowedObjects AllowedObjects;
};

struct ZoneInfo
{
	ZoneShape Shape;
	Team Team;
};

struct DestructibleData
{
	float Health;
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
		static const std::unordered_map<std::string, CrateType> map =
		{
			// Objectives.
			{ TagName::Objective::FlagStand,			CrateType::FlagStand			},
			{ TagName::Objective::CapturePlate,			CrateType::CapturePlate			},
			{ TagName::Objective::HillMarker,			CrateType::HillMarker			},
			
			// Teleports.
			{ TagName::Teleport::TeleporterSender,		CrateType::TeleporterSender		},
			{ TagName::Teleport::TeleporterReceiver,	CrateType::TeleporterReceiver	},
			{ TagName::Teleport::TeleporterTwoWay,		CrateType::TeleporterTwoWay		},
			
			// Palletes.
			{ TagName::Palletes::Pallet,				CrateType::Pallete,				},
			{ TagName::Palletes::PalletLarge,			CrateType::Pallete,				},
			
			// Explosives.
			{ TagName::Explosive::FusionCoil,			CrateType::FusionCoil			},
			{ TagName::Explosive::Landmine,				CrateType::Landmine				},
			{ TagName::Explosive::PlasmaBattery,		CrateType::PlasmaBattery		},
			{ TagName::Explosive::PropaneTank,			CrateType::PropaneTank			},
			
			// Lift.
			{ TagName::Lift::ManCannon,					CrateType::Lift					},
			{ TagName::Lift::ManCannonHeavy,			CrateType::Lift					},
			{ TagName::Lift::ManCannonLight,			CrateType::Lift					},
			{ TagName::Lift::VehicleManCannon,			CrateType::Lift					},
			{ TagName::Lift::GravityLift,				CrateType::Lift					},
			{ TagName::Lift::ManCannonMCC,				CrateType::Lift					},
			{ TagName::Lift::ManCannonHeavyMCC,			CrateType::Lift					},
			{ TagName::Lift::ManCannonLightMCC,			CrateType::Lift					},
			{ TagName::Lift::ForerunnerGravityLift,		CrateType::Lift					},
			{ TagName::Lift::ForerunnerGravityLiftTall,	CrateType::Lift					},
			{ TagName::Lift::ManCannonHumanMCC,			CrateType::Lift					},

			// Shield.
			{ TagName::Shield::OneWayDoorSmall,				CrateType::Shield			},
			{ TagName::Shield::OneWayDoorMedium,			CrateType::Shield			},
			{ TagName::Shield::OneWayDoorLarge,				CrateType::Shield			},
			{ TagName::Shield::TwoWayDoorSmall,				CrateType::Shield			},
			{ TagName::Shield::TwoWayDoorMedium,			CrateType::Shield			},
			{ TagName::Shield::TwoWayDoorLarge,				CrateType::Shield			},
			{ TagName::Shield::OneWayDoorXSmall,			CrateType::Shield			},
			{ TagName::Shield::OneWayDoorGarage,			CrateType::Shield			},
			{ TagName::Shield::ShieldWallSmall,				CrateType::Shield			},
			{ TagName::Shield::ShieldWallMedium,			CrateType::Shield			},
			{ TagName::Shield::ShieldWallLarge,				CrateType::Shield			},
			{ TagName::Shield::ShieldWallXLarge,			CrateType::Shield			},
			{ TagName::Shield::ShieldDoorSmall,				CrateType::Shield			},
			{ TagName::Shield::ShieldDoorMedium,			CrateType::Shield			},
			{ TagName::Shield::ShieldDoorLarge,				CrateType::Shield			},
			{ TagName::Shield::HangarShieldDoorSmall,		CrateType::Shield			},
			{ TagName::Shield::HangarShieldDoorSmallSolid,	CrateType::Shield			},
			{ TagName::Shield::HangarShieldDoorLarge,		CrateType::Shield			},
			{ TagName::Shield::HangarShieldDoorLargeSolid,	CrateType::Shield			},
		};

		auto it = map.find(tagName);
		return it != map.end() ? it->second : CrateType::Unknown;
	}

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
		using namespace ObjectOffsets;

		static const std::unordered_map<CrateType, ZoneOffsets> map =
		{
			{ 
				CrateType::FlagStand,    
				{ 
					Crate::ObjectiveSpawn::Radius,    
					Crate::ObjectiveSpawn::Lenght,    
					Crate::ObjectiveSpawn::Top,    
					Crate::ObjectiveSpawn::Bottom,    
					Crate::ObjectiveSpawn::ShapeType,    
					Crate::ObjectiveSpawn::Team    
				} 
			},
			{ 
				CrateType::CapturePlate, 
				{ 
					Crate::ObjectiveSpawn::Radius,	
					Crate::ObjectiveSpawn::Lenght,		
					Crate::ObjectiveSpawn::Top, 
					Crate::ObjectiveSpawn::Bottom, 
					Crate::ObjectiveSpawn::ShapeType, 
					Crate::ObjectiveSpawn::Team 
				} 
			},
			{ 
				CrateType::HillMarker,   
				{ 
					Crate::HillMarker::Radius,   
					Crate::HillMarker::Lenght,   
					Crate::HillMarker::Top,   
					Crate::HillMarker::Bottom,   
					Crate::HillMarker::ShapeType,   
					Crate::HillMarker::Team   
				} 
			},
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

	std::optional<ZoneInfo> Zone;
	std::optional<TeleporterData> Teleporter;
	std::optional<DestructibleData> Destructible;
	std::optional<LiftData> Lift;
	std::optional<ShieldData> Shield;
};