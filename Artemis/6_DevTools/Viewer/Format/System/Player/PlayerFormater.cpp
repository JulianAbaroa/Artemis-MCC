module DevTools.Viewer.Format.System;
import :Player;

namespace DevTools::Viewer::Format::System
{
    auto PlayerFormater::ConnectionToString(ConnectionState connection) -> const char*
    {
        switch (connection)
        {
        case ConnectionState::Connected:    return "Connected";
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::Connecting:   return "Connecting";
        default:                            return "Unknown";
        }
    }

    auto PlayerFormater::PowerupToString(Powerup powerup) -> const char*
    {
        switch (powerup)
        {
        case Powerup::None:   return "None";
        case Powerup::Red:    return "Red";
        case Powerup::Blue:   return "Blue";
        case Powerup::Yellow: return "Yellow";
        default:              return "Unknown";
        }
    }

    auto PlayerFormater::ZoomToString(ZoomLevel zoom) -> const char*
    {
        switch (zoom)
        {
        case ZoomLevel::None:  return "None";
        case ZoomLevel::Zoom1: return "Zoom 1";
        case ZoomLevel::Zoom2: return "Zoom 2";
        default:               return "Unknown";
        }
    }

    auto PlayerFormater::GrenadeToString(GrenadeType grenade) -> const char*
    {
        switch (grenade)
        {
        case GrenadeType::Frag:   return "Frag";
        case GrenadeType::Plasma: return "Plasma";
        default:                  return "Unknown";
        }
    }

    auto PlayerFormater::GroundToString(GroundKind ground) -> const char*
    {
        switch (ground)
        {
        case GroundKind::Air:      return "Air";
        case GroundKind::Static:   return "Static";
        case GroundKind::Object:   return "Object";
        case GroundKind::Attached: return "Attached";
        default:                   return "Unknown";
        }
    }

    auto PlayerFormater::InfiniteAmmoToString(InfiniteAmmoMode mode) -> const char*
    {
        switch (mode)
        {
        case InfiniteAmmoMode::Off:        return "Off";
        case InfiniteAmmoMode::On:         return "On";
        case InfiniteAmmoMode::Bottomless: return "Bottomless";
        default:                           return "Unknown";
        }
    }

    auto PlayerFormater::AbilityUsageToString(AbilityUsageMode mode) -> const char*
    {
        switch (mode)
        {
        case AbilityUsageMode::Disabled:          return "Disabled";
        case AbilityUsageMode::NotWithObjectives: return "Not with objectives";
        case AbilityUsageMode::Enabled:           return "Enabled";
        default:                                  return "Unknown";
        }
    }

    auto PlayerFormater::VehicleUseToString(VehicleUseMode mode) -> const char*
    {
        switch (mode)
        {
        case VehicleUseMode::Blocked:       return "Blocked";
        case VehicleUseMode::PassengerOnly: return "Passenger only";
        case VehicleUseMode::DriverOnly:    return "Driver only";
        case VehicleUseMode::GunnerOnly:    return "Gunner only";
        case VehicleUseMode::NoPassenger:   return "No passenger";
        case VehicleUseMode::NoDriver:      return "No driver";
        case VehicleUseMode::NoGunner:      return "No gunner";
        case VehicleUseMode::Full:          return "Full";
        default:                            return "Unknown";
        }
    }

    auto PlayerFormater::IdentityVisibilityToString(IdentityVisibility visibility) -> const char*
    {
        switch (visibility)
        {
        case IdentityVisibility::Hidden:   return "Hidden";
        case IdentityVisibility::Allies:   return "Allies";
        case IdentityVisibility::Everyone: return "Everyone";
        default:                           return "Unknown";
        }
    }

    auto PlayerFormater::RadarModeToString(RadarDisplayMode mode) -> const char*
    {
        switch (mode)
        {
        case RadarDisplayMode::Off:      return "Off";
        case RadarDisplayMode::Allies:   return "Allies";
        case RadarDisplayMode::Normal:   return "Normal";
        case RadarDisplayMode::Enhanced: return "Enhanced";
        default:                         return "Unknown";
        }
    }
}