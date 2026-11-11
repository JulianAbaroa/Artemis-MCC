export module Views.Oracle.Type:Sample;

import Common.Math.Type;
import Blam.MapModel.World.Type;
import Blam.Layout.Collision.Type;
import Views.Raycast.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::World::Type::Raycast::Hit;
    using Blam::Layout::Collision::Type::Result::CollisionResult;
    using Views::Raycast::Type::RaycastHit;
}

export namespace Views::Oracle::Type::Sample
{
    // Where a ray of the oracle comes from.
    enum class RaySource : std::uint8_t
    {
        // A ray of the tick: the aim ray or a perception ray, already cast by Artemis against the world and its dynamic objects.
        Tick = 0,

        // A ray from the eye of the player in a random direction.
        Random = 1,
    };

    // Whether a hit is part of the map or an object.
    enum class HitClass : std::uint8_t
    {
        None = 0,
        Static = 1,
        Dynamic = 2,
    };

    // One ray with what the engine and Artemis answered.
    struct OracleSample
    {
        RaySource Source{ RaySource::Tick };

        Vec3 Origin{};
        Vec3 Direction{};
        float MaxDistance{ 0.0f };

        // The engine with the structure and the instanced geometry only, and with every object type too.
        CollisionResult EngineStatic{};
        CollisionResult EngineObjects{};
        std::uint32_t EngineStaticNanoseconds{ 0 };
        std::uint32_t EngineObjectsNanoseconds{ 0 };

        // Artemis against the static world, cast again for the same ray when the oracle read the answer.
        Hit OursStatic{};

        // The ray as the tick published it (static world and dynamic objects), for the rays of the tick only.
        // note: It is one or two ticks older than the engine answer: the engine traced after the tick was handed over.
        std::optional<RaycastHit> OursTick{};
    };

    // How the engine and Artemis differ in one ray, in one of the two comparisons.
    struct RayDifference
    {
        // False when the comparison does not apply (no tick answer for a random ray).
        bool IsCompared{ false };

        bool IsEngineHit{ false };
        bool IsOursHit{ false };
        float EngineDistance{ 0.0f };
        float OursDistance{ 0.0f };

        // One hit and not the other.
        bool IsHitDifferent{ false };

        // Both hit and the distances differ by more than the tolerance.
        bool IsDistanceDifferent{ false };
        float DistanceDelta{ 0.0f };

        // Both hit and one is a surface of the map and the other an object.
        bool IsKindDifferent{ false };

        // Both hit the same kind of object but not the same one.
        bool IsObjectDifferent{ false };

        // Both hit the map and the material or the surface flags differ.
        bool IsMaterialDifferent{ false };
        bool IsFlagsDifferent{ false };

        auto IsAnyDifferent() const -> bool
        {
            return IsHitDifferent || IsDistanceDifferent || IsKindDifferent || IsObjectDifferent || IsMaterialDifferent || IsFlagsDifferent;
        }
    };

    // The two comparisons of a ray: the engine without objects against Artemis, and the engine with objects against the tick.
    struct OracleComparison
    {
        RayDifference Static{};
        RayDifference Objects{};
    };
}