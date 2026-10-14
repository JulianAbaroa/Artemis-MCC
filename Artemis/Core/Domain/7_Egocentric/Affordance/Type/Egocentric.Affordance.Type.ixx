export module Egocentric.Affordance.Type;

import Common.Math.Type;
import Common.Semantic.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Common::Semantic::Type::Function;
}

export namespace Egocentric::Affordance::Type
{
    enum class Behavior : std::uint8_t
    {
        None,
        Pickup,
        EnterVehicle,
        Avoid,
        Interact,
    };

    enum class Activation : std::uint8_t
    {
        None,
        KeyPress,
        Proximity,
    };

    struct SeatStatus
    {
        std::string SeatName;

        bool IsHijackerSlot = false;
        bool IsOccupied = false;

        std::uint32_t OccupyingBipedHandle = 0xFFFFFFFF;

        // TODO: resolve from ModeGeometry markers when available.
        Vec3 SeatWorldPosition{};

        float DistanceToPlayer = 0.0f;
    };

    struct Affordance
    {
        std::uint32_t Handle = 0xFFFFFFFF;
        ::Function Function{ ::Function::Unknown };

        // Value of the subtype enum of the function (Subtype::<Function>). 0 is None.
        std::uint8_t Subtype{};

        Vec3 Position{};

        float DistanceToPlayer = 0.0f;
        bool IsEngineSelected = false;

        std::vector<Behavior> Behaviors;
        Activation Activation = Activation::None;

        std::vector<SeatStatus> Seats;
        std::vector<uint32_t> ChildHandles;
    };
}