export module Egocentric.Affordance.Type;

import Common.Math.Type;
import Relations.Classifier.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Role = Relations::Classifier::Type::Role;
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
        Role Role = Role::Unknown;

        Vec3 Position{};

        float DistanceToPlayer = 0.0f;
        bool IsEngineSelected = false;

        std::vector<Behavior> Behaviors;
        Activation Activation = Activation::None;

        std::vector<SeatStatus> Seats;
        std::vector<uint32_t> ChildHandles;
    };
}