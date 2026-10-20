export module Environment.Details.Type:Lift;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace Environment::Details::Type
{
    // Launch of one entity with Function Lift (D91).
    // note: The kind (Vertical, Curved, Redirected) is the subtype of the entity and the pose is in Entities.
    // The force waits for A12 (E5).
    struct LiftDetail
    {
        std::uint32_t Handle{};

        // Direction of the launch, world space. Zero when the lift has no subtype.
        // note: Provisional until A12. Vertical is +Z, Curved is Forward and Redirected is Forward x Up. Some lifts launch the other way.
        Vec3 LaunchDirection{};
    };

    struct LiftDetails : std::unordered_map<std::uint32_t, LiftDetail>
    {
        using unordered_map::unordered_map;
    };
}