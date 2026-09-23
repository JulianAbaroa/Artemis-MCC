export module Resolved.World.System:Coll;

import Common.Math.Type;
import Map.Reader.Type;
import Map.Tag.Type;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
	class CollBuilder
	{
	private:
		using Vec3 = Common::Math::Type::Vec3;
		using Node = Common::Math::Type::Node;
		using CollObject = Map::Tag::Type::Coll::Object::CollObject;
		using Coll_Regions_Permutations_BspsObject = Map::Tag::Type::Coll::Object::Coll_Regions_Permutations_BspsObject;
		using ResolvedColl = Resolved::World::Type::Coll::Coll;

	public:
		CollBuilder() = default;
		~CollBuilder() = default;

		auto Build(const CollObject& coll) -> ResolvedColl;

	private:

		auto BuildMeshes(const CollObject& coll, ResolvedColl& out) -> void;
		auto BuildNodes(const CollObject& coll, ResolvedColl& out) -> void;
		auto BuildBounds(ResolvedColl& out) -> void;

		// --- Helpers ---
		auto MakeVec3(const Map::Reader::Type::Structure::Primitive::Vec3& v) -> Vec3;

		auto CollectSurfaceVertexIndices(
			const Coll_Regions_Permutations_BspsObject& bsp,
			std::int32_t surfaceIndex) -> std::vector<std::int32_t>;
	};
}