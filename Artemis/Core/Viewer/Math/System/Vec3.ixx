export module Viewer.Math.System;

import Common.Math.Type;
import std;

export namespace Viewer::Math::System
{
	using Vec3 = Common::Math::Type::Vec3;

	inline auto Add(const Vec3& a, const Vec3& b) -> Vec3
	{
		return Vec3{ a.X + b.X, a.Y + b.Y, a.Z + b.Z };
	}

	inline auto Subtract(const Vec3& a, const Vec3& b) -> Vec3
	{
		return Vec3{ a.X - b.X, a.Y - b.Y, a.Z - b.Z };
	}

	inline auto Scale(const Vec3& v, float scalar) -> Vec3
	{
		return Vec3{ v.X * scalar, v.Y * scalar, v.Z * scalar };
	}

	inline auto Dot(const Vec3& a, const Vec3& b) -> float
	{
		return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
	}

	inline auto Cross(const Vec3& a, const Vec3& b) -> Vec3
	{
		return Vec3{
			a.Y * b.Z - a.Z * b.Y,
			a.Z * b.X - a.X * b.Z,
			a.X * b.Y - a.Y * b.X };
	}

	inline auto Length(const Vec3& v) -> float
	{
		return std::sqrt(Dot(v, v));
	}

	inline auto Normalize(const Vec3& v, const Vec3& fallback = Vec3{ 0.0f, 0.0f, 1.0f }) -> Vec3
	{
		const float length = Length(v);
		if (length < 1e-6f) return fallback;

		return Scale(v, 1.0f / length);
	}
}