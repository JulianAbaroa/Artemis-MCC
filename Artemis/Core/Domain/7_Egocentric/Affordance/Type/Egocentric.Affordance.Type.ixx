export module Egocentric.Affordance.Type;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Function;

    using Common::Engine::Type::k_NoHandle;
}

export namespace Egocentric::Affordance::Type
{
    enum class AffordanceBehavior : std::uint8_t
    {
        None,
        Pickup,
        EnterVehicle,
        Avoid,
        Interact,
    };

    enum class AffordanceActivation : std::uint8_t
    {
        None,
        KeyPress,
        Proximity,
    };

    struct SeatStatus
    {
        std::string SeatName{};

        bool IsHijackerSlot = false;
        bool IsOccupied = false;

        std::uint32_t OccupyingBipedHandle = k_NoHandle;

        // TODO: resolve from ModeGeometry markers when available.
        Vec3 SeatWorldPosition{};

        float DistanceToPlayer = 0.0f;
    };

    struct Affordance
    {
        std::uint32_t Handle = k_NoHandle;
        ::Function Function{ ::Function::Unknown };

        // Value of the subtype enum of the function (Subtype::<Function>). 0 is None.
        std::uint8_t Subtype{};

        Vec3 Position{};

        float DistanceToPlayer = 0.0f;
        bool IsEngineSelected = false;

        std::vector<AffordanceBehavior> Behaviors{};
        AffordanceActivation Activation = AffordanceActivation::None;

        std::vector<SeatStatus> Seats{};
        std::vector<uint32_t> ChildHandles{};
    };

    // Affordances of a view.
    struct Affordances : std::vector<Affordance>
    {
        using vector::vector;
    };
}