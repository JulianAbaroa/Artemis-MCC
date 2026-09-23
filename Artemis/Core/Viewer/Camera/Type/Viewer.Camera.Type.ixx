export module Viewer.Camera.Type;

import Common.Math.Type;
import std;

export namespace Viewer::Camera::Type
{
	using Vec2 = Common::Math::Type::Vec2;
	using Vec3 = Common::Math::Type::Vec3;

	using Matrix = std::array<float, 16>;

	enum class Key : std::uint8_t
	{
		Forward,
		Backward,
		Left,
		Right,
		Up,
		Down,
		Fast,
		Slow,

		Count
	};

	struct Lens
	{
		float FovY{ 1.0472f };
		float Near{ 0.05f };
		float Far{ 10000.0f };
	};

	struct Viewport
	{
		Vec2 Position{};
		Vec2 Size{};
	};

	struct Ray
	{
		Vec3 Origin{};
		Vec3 Direction{};
	};
}