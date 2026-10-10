export module Egocentric.Raycast.Type:Raycasts;

import Common.Math.Type;
import Resolved.Definitions.Type;
import Environment.Collidable.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using Collidable = Environment::Collidable::Type::Collidable;
	using Mesh = Resolved::Definitions::Type::Coll::Mesh;
}

export namespace Egocentric::Raycast::Type
{
	enum class HitKind : std::uint8_t
	{
		None = 0,
		Static = 1,
		Dynamic = 2,
	};

	struct RaycastHit
	{
		bool Hit{ false };
		HitKind Kind{ HitKind::None };

		float Distance{ 0.0f };
		Vec3 Point{};

		std::uint32_t ObjectHandle{ 0 };

		// Region of the collision model that was hit, or -1 when no dynamic part was.
		std::int16_t RegionIndex{ -1 };

		Vec3 Origin{};
		Vec3 Direction{};
		float MaxDistance{ 0.0f };
	};

	// Part of a dynamic object, prepared for the rays of one tick.
	struct CachedPart
	{
		const Mesh* Source{ nullptr };

		// World to local space, as the rows of a 3x4 matrix.
		std::array<float, 12> ToLocal{};

		// Sphere that encloses the part in the world.
		Vec3 Center{};
		float Radius{ 0.0f };
	};

	struct CachedDynamic
	{
		Collidable Data{};
		std::vector<CachedPart> Parts{};
		float BoundingRadius{ 0.0f };
	};

	struct PerceptionOrigin
	{
		Vec3 Position{};
		Vec3 Forward{};
		bool Valid{ false };
	};

	struct Raycasts
	{
		RaycastHit AimHit{};

		std::vector<RaycastHit> PerceptionHits{};
	};
}