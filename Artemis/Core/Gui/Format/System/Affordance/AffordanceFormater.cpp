module Gui.Format.System;
import :Affordance;

namespace Gui::Format::System
{
    auto AffordanceFormater::BehaviorToString(AffordanceBehavior behavior) -> const char*
    {
        switch (behavior)
        {
        case AffordanceBehavior::Pickup:       return "Pickup";
        case AffordanceBehavior::EnterVehicle: return "EnterVehicle";
        case AffordanceBehavior::Avoid:        return "Avoid";
        case AffordanceBehavior::Interact:     return "Interact";
        default:                     return "Unknown";
        }
    }

    auto AffordanceFormater::ActivationToString(AffordanceActivation activation) -> const char*
    {
        switch (activation)
        {
        case AffordanceActivation::None:      return "None";
        case AffordanceActivation::KeyPress:  return "KeyPress";
        case AffordanceActivation::Proximity: return "Proximity";
        default:                    return "Unknown";
        }
    }
}