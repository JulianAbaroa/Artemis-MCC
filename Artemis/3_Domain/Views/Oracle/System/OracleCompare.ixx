export module Views.Oracle.System:Compare;

import Common.Math.Type;
import Blam.Layout.Collision.Type;
import Blam.Runtime.Object.Type;
import Views.Oracle.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::Layout::Collision::Type::Result::CollisionResult;
    using Blam::Runtime::Object::Type::Alive::ObjectTable;
    using Views::Oracle::Type::Sample::HitClass;
    using Views::Oracle::Type::Sample::OracleComparison;
    using Views::Oracle::Type::Sample::OracleSample;
    using Views::Oracle::Type::Sample::RayDifference;
}

export namespace Views::Oracle::System
{
    // return: Whether the engine hit nothing, a surface of the map or an object.
    auto ClassifyHit(const CollisionResult& result) -> HitClass;

    // return: How the engine without objects and Artemis against the static world differ for the ray.
    auto CompareStatic(const OracleSample& sample) -> RayDifference;

    // return: How the engine with objects and the ray of the tick differ. Not compared when the sample has no ray of the tick.
    // note: The engine traced one or two ticks after the tick, so a moving object can differ.
    auto CompareObjects(const OracleSample& sample) -> RayDifference;

    auto Compare(const OracleSample& sample) -> OracleComparison;

    // return: The root of the parent chain of an object, which is what the client of the game passes as the object its rays ignore. The
    // handle itself if it has no parent or is not in the table.
    auto ResolveRootObject(const ObjectTable& objects, std::uint32_t handle) -> std::uint32_t;

    // return: A direction of unit length, uniform over the sphere.
    auto RandomDirection(std::mt19937& generator) -> Vec3;
}