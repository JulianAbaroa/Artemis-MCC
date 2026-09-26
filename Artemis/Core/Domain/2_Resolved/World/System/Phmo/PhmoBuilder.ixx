export module Resolved.World.System:Phmo;

import Common.Math.Type;
import Map.Tag.Type;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
    class PhmoBuilder
    {
    private:
        using Node = Common::Math::Type::Node;
        using PhmoObject = Map::Tag::Type::Phmo::Object::PhmoObject;
        using Phmo_BoxesEntry = Map::Tag::Type::Phmo::Structure::Phmo_BoxesEntry;
        using Phmo_PillsEntry = Map::Tag::Type::Phmo::Structure::Phmo_PillsEntry;
        using Phmo_SpheresEntry = Map::Tag::Type::Phmo::Structure::Phmo_SpheresEntry;
        using Phmo_PolyhedraEntry = Map::Tag::Type::Phmo::Structure::Phmo_PolyhedraEntry;
        using Phmo_MultiSpheresEntry = Map::Tag::Type::Phmo::Structure::Phmo_MultiSpheresEntry;
        using ResolvedPhmo = Resolved::World::Type::Phmo::Phmo;
        using Shape = Resolved::World::Type::Phmo::Shape;
		using ShapeType = Resolved::World::Type::Phmo::ShapeType;
        using MultiSphere = Resolved::World::Type::Phmo::MultiSphere;

    public:
        PhmoBuilder() = default;
        ~PhmoBuilder() = default;

        auto Build(const PhmoObject& phmo) -> ResolvedPhmo;

    private:
        auto BuildRigidBodies(const PhmoObject& phmo, ResolvedPhmo& out) -> void;
        auto BuildNodes(const PhmoObject& phmo, ResolvedPhmo& out) -> void;
        auto BuildBounds(ResolvedPhmo& out) -> void;

        auto ResolveShapes(const PhmoObject& phmo, std::uint16_t shapeType,
            std::int16_t shapeIndex) -> std::vector<Shape>;

        auto ResolvePrimitive(const PhmoObject& phmo, std::uint16_t shapeType,
            std::int16_t shapeIndex, Shape& out) -> bool;

        auto MakeSphere(const Phmo_SpheresEntry& src) -> Shape;
        auto MakePill(const Phmo_PillsEntry& src) -> Shape;
        auto MakeBox(const Phmo_BoxesEntry& src) -> Shape;
        auto MakePolyhedron(const PhmoObject& phmo, const Phmo_PolyhedraEntry& src, int16_t polyIndex) -> Shape;
        auto MakeMultiSphere(const Phmo_MultiSpheresEntry& src) -> Shape;
    };
}