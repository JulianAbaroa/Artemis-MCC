export module Resolved.World.System:Sbsp;

import Common.Math.Type;
import Map.Reader.Type;
import Map.Tag.Type;
import Map.Tag.State;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
    class SbspBuilder
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using RangeF = Map::Reader::Type::Structure::Primitive::RangeF;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using ResolvedSbsp = Resolved::World::Type::Sbsp::Sbsp;
        using Cluster = Resolved::World::Type::Sbsp::Cluster;
		using Portal = Resolved::World::Type::Sbsp::Portal;
        using Marker = Resolved::World::Type::Sbsp::Marker;
        using CollisionMaterial = Resolved::World::Type::Sbsp::CollisionMaterial;

    public:
        SbspBuilder() = default;
        ~SbspBuilder() = default;

        auto Build(const SbspObject& sbsp, std::int32_t sbspIndex) -> ResolvedSbsp;

    private:
        auto BuildWorldBounds(const SbspObject& sbsp, ResolvedSbsp& out) -> void;
        auto BuildPortals(const SbspObject& sbsp, ResolvedSbsp& out) -> void;
        auto BuildClusters(const SbspObject& sbsp, ResolvedSbsp& out) -> void;
        auto BuildClusterAdjacency(const SbspObject& sbsp, ResolvedSbsp& out) -> void;
        auto BuildInstancedGeometry(const SbspObject& sbsp, ResolvedSbsp& out) -> void;
        auto BuildCollisionMaterials(const SbspObject& sbsp, ResolvedSbsp& out) -> void;
        auto BuildMarkers(const SbspObject& sbsp, ResolvedSbsp& out) -> void;

        // --- Helpers ---
        auto RangeFCenter(const RangeF& x, const RangeF& y, const RangeF& z) -> Vec3;
    };
}