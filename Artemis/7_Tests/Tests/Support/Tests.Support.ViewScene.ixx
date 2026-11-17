export module Tests.Support.ViewScene;

import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Entities.Type;
import std;

// Made-up objects for the tests of the views per agent: a plane the rays cross, a live object and its entity.

namespace
{
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Function;
    using Common::Team::Type::Team;
    using ::Blam::MapModel::Definitions::Type::Coll::Coll;
    using ::Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using ::Blam::Runtime::Object::Type::Alive::AliveObject;
    using Perception::Entities::Type::Entity;
}

export namespace Tests::Support::ViewScene
{
    // return: A plane of 40 x 40 m that crosses one axis at the value (0 x, 1 y, 2 z) in the local space of the object.
    auto MakePlane(std::string_view name, int axis, float value) -> Coll
    {
        const auto point{ [&](float a, float b) {
            return axis == 0 ? Vec3{ value, a, b } : (axis == 1 ? Vec3{ a, value, b } : Vec3{ a, b, value });
        } };

        Coll coll{};
        coll.TagName = std::string{ name };
        CollMesh mesh{};
        mesh.PermutationIndex = 0;
        mesh.Triangles.push_back(Triangle{ point(-20.0f, -20.0f), point(20.0f, -20.0f), point(20.0f, 20.0f) });
        mesh.Triangles.push_back(Triangle{ point(-20.0f, -20.0f), point(20.0f, 20.0f), point(-20.0f, 20.0f) });
        mesh.LocalMin = point(-20.0f, -20.0f);
        mesh.LocalMax = point(20.0f, 20.0f);
        coll.Meshes.push_back(mesh);
        coll.DefaultPermutationIndex = {};
        coll.BoundsMin = { -20.0f, -20.0f, -20.0f };
        coll.BoundsMax = { 20.0f, 20.0f, 20.0f };
        return coll;
    }

    // return: A live object at the center, facing +X with +Z up.
    auto MakeObject(std::uint32_t handle, std::uint32_t datum, std::string_view tag, const Vec3& center) -> AliveObject
    {
        AliveObject object{};
        object.Handle = handle;
        object.DatumIndex = datum;
        object.TagName = std::string{ tag };
        object.BoundingCenter = center;
        object.WorldOrigin = center;
        object.Forward = { 1.0f, 0.0f, 0.0f };
        object.Up = { 0.0f, 0.0f, 1.0f };
        return object;
    }

    auto MakeEntity(std::uint32_t handle, Function function, Team team) -> Entity
    {
        Entity entity{};
        entity.Handle = handle;
        entity.Function = function;
        entity.Team = team;
        return entity;
    }
}