export module Environment.Aim.Type;

import Common.Math.Type;
import Resolved.World.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using AimSource = Resolved::World::Type::ModelLink::AnchorSource;
}

export namespace Environment::Aim::Type
{
	// World-space aim point of one damage section, recomputed every tick
	// from its resolved anchor (mode node + local offset) and the bone matrices
	struct SectionAim
	{
		Vec3 Position{};
		float Radius{};
		AimSource Source{ AimSource::None };
		bool Valid{ false };
	};

	struct Aim
	{
		std::uint32_t Handle{};

		// Same index as the Resolved Vitality sections of the object (and Health.SectionVitalities)
		std::vector<SectionAim> Sections{};
	};

	using Aims = std::unordered_map<std::uint32_t, Aim>;
}