module UI.Format.System;
import :Fixtures;

namespace UI::Format::System
{
	auto FixturesFormater::ZoneKindToString(ZoneKind kind) -> const char*
	{
		switch (kind)
		{
		case ZoneKind::None:		return "None";
		case ZoneKind::Cylinder:	return "Cylinder";
		case ZoneKind::Box:			return "Box";
		default:					return "Unknown";
		}
	}

	auto FixturesFormater::AngleKindToString(AngleKind kind) -> const char*
	{
		switch (kind)
		{
		case AngleKind::Curved:		return "Curved";
		case AngleKind::Vertical:	return "Vertical";
		case AngleKind::Redirected:	return "Redirected";
		default:					return "Unknown";
		}
	}

	auto FixturesFormater::ForceKindToString(ForceKind kind) -> const char*
	{
		switch (kind)
		{
		case ForceKind::Default:	return "Default";
		case ForceKind::Heavy:		return "Heavy";
		case ForceKind::Light:		return "Light";
		case ForceKind::Vehicle:	return "Vehicle";
		default:					return "Unknown";
		}
	}

	auto FixturesFormater::DestructibleKindToString(DestructibleKind kind) -> const char*
	{
		switch (kind)
		{
		case DestructibleKind::Explosive:		return "Explosive";
		case DestructibleKind::Pallet:			return "Pallet";
		case DestructibleKind::PortableShield:	return "Portable Shield";
		default:								return "Unknown";
		}
	}

	auto FixturesFormater::TeleporterKindToString(TeleporterKind kind) -> const char*
	{
		switch (kind)
		{
		case TeleporterKind::Receiver:		return "Receiver";
		case TeleporterKind::Sender:		return "Sender";
		case TeleporterKind::TwoWay:		return "TwoWay";
		default:							return "Unknown";
		}
	}
}