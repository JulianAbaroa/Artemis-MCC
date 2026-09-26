export module Common.Math.System;

import Common.Math.Type;

export namespace Common::Math::System
{
	using Vec3 = Common::Math::Type::Vec3;

	auto Add(const Vec3& a, const Vec3& b) -> Vec3;
	auto Subtract(const Vec3& a, const Vec3& b) -> Vec3;
	auto Scale(const Vec3& v, float scalar) -> Vec3;
	auto Cross(const Vec3& a, const Vec3& b) -> Vec3;
	auto Dot(const Vec3& a, const Vec3& b) -> float;
	auto Length(const Vec3& v) -> float;
	auto Distance(const Vec3& a, const Vec3& b) -> float;

	auto Normalize(const Vec3& v, const Vec3& fallback = Vec3{ 0.0f, 0.0f, 1.0f }) -> Vec3;

	auto BuildFrame(const Vec3& forwardIn, Vec3& outForward, Vec3& outRight,
		Vec3& outUp, const Vec3& normalizeFallback = Vec3{ 0.0f, 0.0f, 0.0f }) -> void;
}