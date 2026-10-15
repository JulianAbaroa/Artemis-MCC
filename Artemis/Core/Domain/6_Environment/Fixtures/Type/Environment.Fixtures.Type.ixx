export module Environment.Fixtures.Type;

import Tables.Object.Type;
import Common.Team.Type;
import Common.Math.Type;
import std;

namespace
{
    namespace Crate = Tables::Object::Type::Crate;

    using Vec3 = Common::Math::Type::Vec3;
    using Team = Common::Team::Type::Team;
}

export namespace Environment::Fixtures::Type
{
    namespace Obstacle
    {
        struct Obstacle
        {
            std::uint32_t Handle;
            std::string TagName;

            Vec3 Position;
            Vec3 Forward;
            Vec3 Up;

            Vec3 LinearVelocity;
            Vec3 AngularVelocity;

            float BoundingRadius;
        };
    }

    namespace Spawn
    {
        enum class Kind : std::uint8_t
        {
            Initial,
            Respawn,
            Invisible,
        };

        struct Spawn
        {
            std::uint32_t Handle;
            std::string TagName;

            Vec3 Position;
            Vec3 Forward;

            Kind Kind;
            Team Team;
        };
    }

    namespace Lift
    {
        struct Lift
        {
            std::uint32_t Handle;
            std::string TagName;

            Vec3 Position;
            Vec3 Forward;
            Vec3 Up;

            Crate::Lift::Angle Angle;
            Crate::Lift::Force Force;

            Vec3 LaunchDirection;
        };
    }

    namespace Shield
    {
        struct Shield
        {
            std::uint32_t Handle;
            std::string TagName;

            Vec3 Position;
            Vec3 Forward;
            Vec3 Up;

            Crate::Shield::Kind Kind;

            std::optional<Vec3> BlockDirection;
        };
    }

    namespace Objective
    {
        struct Objective
        {
            std::uint32_t Handle;
            std::string TagName;

            Vec3 Position;
            Vec3 Forward;
            Vec3 Up;

            Vec3 LinearVelocity;
            Vec3 AngularVelocity;

            Team Team;
            bool IsEquipped;
            std::uint32_t CarrierHandle;
        };
    }

    namespace Destructible
    {
        enum class Kind : uint8_t
        {
            Pallet,
            Explosive,
            PortableShield
        };

        struct Destructible
        {
            std::uint32_t Handle;
            std::string TagName;

            Vec3 Position;
            Vec3 Forward;
            Vec3 Up;

            Vec3 LinearVelocity;
            Vec3 AngularVelocity;

            Kind Kind;
            float Health;
        };
    }

    struct Fixtures
    {
        std::vector<Obstacle::Obstacle> Obstacles;
        std::vector<Spawn::Spawn> Spawns;
        std::vector<Lift::Lift> Lifts;
        std::vector<Shield::Shield> Shields;
        std::vector<Objective::Objective> Objectives;
        std::vector<Destructible::Destructible> Destructibles;
    };
}