module UI.Format.System;
import :Affordance;

namespace UI::Format::System
{
	auto AffordanceFormater::BehaviorToString(Behaviour behaviour) -> const char*
	{
		switch (behaviour)
		{
		case Behaviour::Pickup:       return "Pickup";
		case Behaviour::EnterVehicle: return "EnterVehicle";
		case Behaviour::Avoid:        return "Avoid";
		case Behaviour::Interact:     return "Interact";
		default:                      return "Unknown";
		}
	}

	auto AffordanceFormater::ActivationToString(Activation activation) -> const char*
	{
		switch (activation)
		{
		case Activation::None:      return "None";
		case Activation::KeyPress:  return "KeyPress";
		case Activation::Proximity: return "Proximity";
		default:                    return "Unknown";
		}
	}
}