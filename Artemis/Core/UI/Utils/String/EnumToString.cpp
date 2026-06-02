#include "pch.h"

#include "EnumToString.h"

const char* EnumToString::ActionStateToString(ActionState state)
{
	switch (state)
	{
	case ActionState::Idle:                 return "Idle";
	case ActionState::Firing:               return "Firing";
	case ActionState::Meleeing:             return "Meleeing";
	case ActionState::Zoomed:               return "Zoomed";
	default:                                return "Unknown";
	}
}

const char* EnumToString::VehicleTypeToString(VehicleType type)
{
	switch (type)
	{
	case VehicleType::Banshee:				return "Banshee";
	case VehicleType::Falcon:				return "Falcon";
	case VehicleType::FalconTurret:			return "Falcon Turret";
	case VehicleType::FalconGrenadeTurret:	return "Falcon Grenade Turret";
	case VehicleType::Ghost:				return "Ghost";
	case VehicleType::Mongoose:				return "Mongoose";
	case VehicleType::Revenant:				return "Revenant";
	case VehicleType::Scorpion:				return "Scorpion";
	case VehicleType::ScorpionTurret:		return "Scorpion Turret";
	case VehicleType::Shade:			return "Shade Turret";
	case VehicleType::Warthog:				return "Warthog";
	case VehicleType::WarthogChaingun:		return "Warthog Chaingun";
	case VehicleType::WarthogGauss:			return "Warthog Gauss";
	case VehicleType::WarthogRocket:		return "Warthog Rocket";
	case VehicleType::Wraith:				return "Wraith";
	case VehicleType::WraithPlasmaTurret:	return "Wraith Plasma Turret";
	default: return "Unknown";
	}
}

const char* EnumToString::TeamToString(Team team)
{
	switch (team)
	{
	case Team::Red:		return "Red";
	case Team::Blue:	return "Blue";
	case Team::Green:	return "Green";
	case Team::Orange:	return "Orange";
	case Team::Purple:	return "Purple";
	case Team::Gold:	return "Gold";
	case Team::Brown:	return "Brown";
	case Team::Pink:	return "Pink";
	case Team::Neutral: return "Neutral";
	default: return "Unknown";
	}
}

const char* EnumToString::ZoomLevelToString(ZoomLevel zoom)
{
	switch (zoom)
	{
	case ZoomLevel::None:	return "None";
	case ZoomLevel::Zoom1:	return "Zoom x1";
	case ZoomLevel::Zoom2:	return "Zoom x2";
	default: return "Unknown";
	}
}

const char* EnumToString::CrateTypeToString(CrateType type)
{
	switch (type)
	{
	case CrateType::FlagStand:			return "Flag Stand";
	case CrateType::CapturePlate:		return "Capture Plate";
	case CrateType::HillMarker:			return "Hill Marker";
	case CrateType::TeleporterSender:	return "Teleporter Sender";
	case CrateType::TeleporterReceiver: return "Teleporter Receiver";
	case CrateType::TeleporterTwoWay:	return "Teleporter Two-Way";
	case CrateType::FusionCoil:			return "Fusion Coil";
	case CrateType::Landmine:			return "Landmine";
	case CrateType::PlasmaBattery:		return "Plasma Battery";
	case CrateType::PropaneTank:		return "Propane Tank";
	default: return "Unknown";
	}
}

const char* EnumToString::SceneryTypeToString(SceneryType type)
{
	switch (type)
	{
	case SceneryType::InitialSpawnPoint:	return "Initial Spawn Point";
	case SceneryType::RespawnPoint:			return "Respawn Point";
	case SceneryType::SafeBoundary:			return "Safe Boundary";
	case SceneryType::SoftSafeBoundary:		return "Soft Safe Boundary";
	case SceneryType::KillBoundary:			return "Kill Boundary";
	case SceneryType::SoftKillBoundary:		return "Soft Kill Boundary";
	default: return "Unknown";
	}
}

const char* EnumToString::ShapeTypeToString(ShapeType type)
{
	switch (type)
	{
	case ShapeType::None:		return "None";
	case ShapeType::Cylinder:	return "Cylinder";
	case ShapeType::Box:		return "Box";
	default:					return "Unknown";
	}
}

const char* EnumToString::AngleTypeToString(AngleType type)
{
	switch (type)
	{
	case AngleType::Curved:		return "Curved";
	case AngleType::Vertical:	return "Vertical";
	case AngleType::Redirected:	return "Redirected";
	default:					return "Unknown";
	}
}

const char* EnumToString::ForceTypeToString(ForceType type)
{
	switch (type)
	{
	case ForceType::Default:	return "Default";
	case ForceType::Heavy:		return "Heavy";
	case ForceType::Light:		return "Light";
	case ForceType::Vehicle:	return "Vehicle";
	default:					return "Unknown";
	}
}

const char* EnumToString::DestructibleTypeToString(DestructibleType type)
{
	switch (type)
	{
	case DestructibleType::Explosive:		return "Explosive";
	case DestructibleType::Pallet:			return "Pallet";
	case DestructibleType::PortableShield:	return "Portable Shield";
	default:								return "Unknown";
	}
}

const char* EnumToString::TeleporterTypeToString(TeleporterType type)
{
	switch (type)
	{
	case TeleporterType::Receiver:		return "Receiver";
	case TeleporterType::Sender:		return "Sender";
	case TeleporterType::TwoWay:		return "TwoWay";
	default:							return "Unknown";
	}
}