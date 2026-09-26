export module Egocentric.Raycast.Type:Raycasts;

import Common.Math.Type;
import Environment.Collidable.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using Collidable = Environment::Collidable::Type::Collidable;
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

		Vec3 Origin{};
		Vec3 Direction{};
		float MaxDistance{ 0.0f };
	};

	struct CachedDynamic
	{
		Collidable Data{};
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