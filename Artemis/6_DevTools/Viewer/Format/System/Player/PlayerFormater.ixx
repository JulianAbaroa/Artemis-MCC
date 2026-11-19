export module DevTools.Viewer.Format.System:Player;

import Common.Semantic.Type;
import Blam.Layout.Player.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Player.Type;
import Perception.Players.Type;

export namespace DevTools::Viewer::Format::System
{
    // Converts the enums of a player, its body and its traits to display text. Unlisted values give "Unknown".
    class PlayerFormater
    {
    private:
        using Powerup = Common::Semantic::Type::Subtype::Powerup;
        using ConnectionState = Blam::Runtime::Player::Type::Alive::ConnectionState;
        using ZoomLevel = Blam::Runtime::Object::Type::Biped::ZoomLevel;
        using GrenadeType = Blam::Runtime::Object::Type::Biped::GrenadeType;
        using GroundKind = Perception::Players::Type::GroundKind;
        using InfiniteAmmoMode = Blam::Layout::Player::Type::Traits::InfiniteAmmoMode;
        using AbilityUsageMode = Blam::Layout::Player::Type::Traits::AbilityUsageMode;
        using VehicleUseMode = Blam::Layout::Player::Type::Traits::VehicleUseMode;
        using IdentityVisibility = Blam::Layout::Player::Type::Traits::IdentityVisibility;
        using RadarDisplayMode = Blam::Layout::Player::Type::Traits::RadarDisplayMode;

    public:
        PlayerFormater() = default;
        ~PlayerFormater() = default;

        static auto ConnectionToString(ConnectionState connection) -> const char*;
        static auto PowerupToString(Powerup powerup) -> const char*;
        static auto ZoomToString(ZoomLevel zoom) -> const char*;
        static auto GrenadeToString(GrenadeType grenade) -> const char*;
        static auto GroundToString(GroundKind ground) -> const char*;
        static auto InfiniteAmmoToString(InfiniteAmmoMode mode) -> const char*;
        static auto AbilityUsageToString(AbilityUsageMode mode) -> const char*;
        static auto VehicleUseToString(VehicleUseMode mode) -> const char*;
        static auto IdentityVisibilityToString(IdentityVisibility visibility) -> const char*;
        static auto RadarModeToString(RadarDisplayMode mode) -> const char*;
    };
}