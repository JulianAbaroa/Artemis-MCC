module UI.Format.System;
import :Object;

namespace UI::Format::System
{
	auto ObjectFormater::ActionStateToString(ActionState state) -> const char*
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

	auto ObjectFormater::VehicleKindToString(VehicleKind kind) -> const char*
	{
		switch (kind)
		{
		case VehicleKind::Banshee:				return "Banshee";
		case VehicleKind::Falcon:				return "Falcon";
		case VehicleKind::FalconTurret:			return "Falcon Turret";
		case VehicleKind::FalconGrenadeTurret:	return "Falcon Grenade Turret";
		case VehicleKind::Ghost:				return "Ghost";
		case VehicleKind::Mongoose:				return "Mongoose";
		case VehicleKind::Revenant:				return "Revenant";
		case VehicleKind::Scorpion:				return "Scorpion";
		case VehicleKind::ScorpionTurret:		return "Scorpion Turret";
		case VehicleKind::ShadeTurret:			return "Shade Turret";
		case VehicleKind::Warthog:				return "Warthog";
		case VehicleKind::WarthogChaingun:		return "Warthog Chaingun";
		case VehicleKind::WarthogGauss:			return "Warthog Gauss";
		case VehicleKind::WarthogRocket:		return "Warthog Rocket";
		case VehicleKind::Wraith:				return "Wraith";
		case VehicleKind::WraithPlasmaTurret:	return "Wraith Plasma Turret";
		default:								return "Unknown";
		}
	}

	auto ObjectFormater::ZoomLevelToString(ZoomLevel zoom) -> const char*
	{
		switch (zoom)
		{
		case ZoomLevel::None:	return "None";
		case ZoomLevel::Zoom1:	return "Zoom x1";
		case ZoomLevel::Zoom2:	return "Zoom x2";
		default:				return "Unknown";
		}
	}

	auto ObjectFormater::CrateKindToString(CrateKind kind) -> const char*
	{
		switch (kind)
		{
		case CrateKind::FlagStand:			return "Flag Stand";
		case CrateKind::CapturePlate:		return "Capture Plate";
		case CrateKind::HillMarker:			return "Hill Marker";
		case CrateKind::TeleportSender:		return "Teleporter Sender";
		case CrateKind::TeleportReceiver:	return "Teleporter Receiver";
		case CrateKind::TeleportTwoWay:		return "Teleporter Two-Way";
		case CrateKind::FusionCoil:			return "Fusion Coil";
		case CrateKind::Landmine:			return "Landmine";
		case CrateKind::PlasmaBattery:		return "Plasma Battery";
		case CrateKind::PropaneTank:		return "Propane Tank";
		default:							return "Unknown";
		}
	}

	auto ObjectFormater::SceneryKindToString(SceneryKind kind) -> const char*
	{
		switch (kind)
		{
		case SceneryKind::InitialSpawnPoint:	return "Initial Spawn Point";
		case SceneryKind::RespawnPoint:			return "Respawn Point";
		case SceneryKind::SafeBoundary:			return "Safe Boundary";
		case SceneryKind::SoftSafeBoundary:		return "Soft Safe Boundary";
		case SceneryKind::KillBoundary:			return "Kill Boundary";
		case SceneryKind::SoftKillBoundary:		return "Soft Kill Boundary";
		default:								return "Unknown";
		}
	}
}