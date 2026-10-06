module Gui.Format.System;
import :Affordance;

namespace Gui::Format::System
{
    auto AffordanceFormater::BehaviorToString(Behavior behavior) -> const char*
    {
        switch (behavior)
        {
        case Behavior::Pickup:       return "Pickup";
        case Behavior::EnterVehicle: return "EnterVehicle";
        case Behavior::Avoid:        return "Avoid";
        case Behavior::Interact:     return "Interact";
        default:                     return "Unknown";
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