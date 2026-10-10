export module Environment.Aim.Type;

import Common.Math.Type;
import Resolved.World.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using AnchorSource = Resolved::World::Type::ModelLink::AnchorSource;
}

export namespace Environment::Aim::Type
{
	struct SectionAim
	{
		Vec3 Position{};
		float Radius{};
		AnchorSource AimSource{ AnchorSource::None };
		bool Valid{ false };
	};

	struct Aim
	{
		std::uint32_t Handle{};

		std::vector<SectionAim> Sections{};
	};
}