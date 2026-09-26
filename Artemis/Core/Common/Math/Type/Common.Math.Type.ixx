export module Common.Math.Type;

import std;

export namespace Common::Math::Type
{
	struct Vec2 { float X{}, Y{}; };
	struct Vec3 { float X{}, Y{}, Z{}; };
	struct Vec4 { float X{}, Y{}, Z{}, W{}; };

	struct Triangle
	{
		Vec3 A{};
		Vec3 B{};
		Vec3 C{};

		std::uint8_t SurfaceFlags{ 0 };
		std::int16_t Material{ -1 };
	};

	struct Node
	{
		std::string Name{};
		std::int16_t ParentIndex{ -1 };
		std::int16_t NextSiblingIndex{ -1 };
		std::int16_t FirstChildIndex{ -1 };

		// T-pose local-space transform.
		Vec3 DefaultTranslation{};
		Vec4 DefaultRotation{};
		float InverseScale{ 1.0f };

		bool DoesNotAnimate{ false };
	};
}