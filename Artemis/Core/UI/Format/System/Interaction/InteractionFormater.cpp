module UI.Format.System;
import :Interaction;

namespace UI::Format::System
{
	auto InteractionFormater::InteractionKindToString(InteractionKind kind) -> const char*
	{
		switch (kind)
		{
		case InteractionKind::None:              return "None";
		case InteractionKind::GrabWeapon:        return "GrabWeapon";
		case InteractionKind::GrabArmorAbility:  return "GrabArmorAbility";
		case InteractionKind::TakeHealthStation: return "TakeHealthStation";
		case InteractionKind::EnterVehicle:      return "EnterVehicle";
		case InteractionKind::Hijack:            return "Hijack";
		case InteractionKind::GrabObjective:     return "GrabObjective";
		default:                                 return "Unknown";
		}
	}

	auto InteractionFormater::InteractionDetailToString(InteractionKind kind,
		InteractionDetail detail) -> const char*
	{
		switch (kind)
		{
		case InteractionKind::GrabWeapon:
			switch (detail)
			{
			case InteractionDetail::GrabWeapon:   return "Grab Weapon";
			case InteractionDetail::ChangeWeapon: return "Change Weapon";
			default:                              return "Unknown";
			}

		case InteractionKind::EnterVehicle:
		case InteractionKind::Hijack:
			switch (detail)
			{
			case InteractionDetail::ZeroSeat:   return "Seat (0)";
			case InteractionDetail::FirstSeat:  return "Seat (1)";
			case InteractionDetail::SecondSeat: return "Seat (2)";
			case InteractionDetail::ThirdSeat:  return "Seat (3)";
			case InteractionDetail::FourthSeat: return "Seat (4)";
			case InteractionDetail::FifthSeat:  return "Seat (5)";
			default:                            return "Unknown Seat";
			}

		case InteractionKind::GrabArmorAbility:  return "Grab Armor Ability";
		case InteractionKind::TakeHealthStation: return "Take Health Station";
		case InteractionKind::GrabObjective:     return "Grab Objective";
		case InteractionKind::None:              return "None";
		default:                                 return "Unknown";
		}
	}
}