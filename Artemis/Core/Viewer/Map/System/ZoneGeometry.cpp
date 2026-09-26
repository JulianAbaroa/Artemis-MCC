module Viewer.Map.System;
import :ZoneGeometry;

import Common.Math.System;
import std;

namespace
{
	namespace VecMath = Common::Math::System;

	using Vertex = Platform::Render::System::GpuPipeline::Vertex;
	using Vec3 = Common::Math::Type::Vec3;
	using Color = Viewer::Map::Type::Color;
	using ZoneKind = Common::ZoneShape::Type::Kind;

	constexpr int k_CylinderSegments = 24;
	constexpr float k_TwoPi = 6.2831853f;

	struct Basis
	{
		Vec3 Right{};
		Vec3 Forward{};
		Vec3 Up{};
	};

	auto MakeBasis(const Vec3& forward, const Vec3& up) -> Basis
	{
		Basis basis;
		basis.Forward = VecMath::Normalize(forward);
		basis.Up = VecMath::Normalize(up);
		basis.Right = VecMath::Normalize(VecMath::Cross(basis.Forward, basis.Up));
		basis.Up = VecMath::Normalize(VecMath::Cross(basis.Right, basis.Forward));
		return basis;
	}

	auto ToWorld(const Vec3& center, const Basis& basis,
		float right, float forward, float up) -> Vec3
	{
		return Vec3{
			center.X + basis.Right.X * right + basis.Forward.X * forward + basis.Up.X * up,
			center.Y + basis.Right.Y * right + basis.Forward.Y * forward + basis.Up.Y * up,
			center.Z + basis.Right.Z * right + basis.Forward.Z * forward + basis.Up.Z * up };
	}

	auto MakeVertex(const Vec3& position, const Color& color) -> Vertex
	{
		return Vertex{ position.X, position.Y, position.Z, color.R, color.G, color.B };
	}
}

namespace Viewer::Map::System::ZoneGeometry
{
	auto AppendSolid(std::vector<Vertex>& out, const Vec3& position,
		const Vec3& forward, const Vec3& up, const ZoneShape& zone,
		const Color& color) -> void
	{
		const Basis basis = MakeBasis(forward, up);

		const float top = zone.Top;
		const float bottom = -zone.Bottom;

		auto world = [&](float right, float forwardOffset, float upOffset) {
			return ToWorld(position, basis, right, forwardOffset, upOffset);
			};

		auto push = [&](const Vec3& point) {
			out.push_back(MakeVertex(point, color));
			};

		if (zone.Kind == ZoneKind::Cylinder)
		{
			const float radius = zone.Radius;

			for (int i = 0; i < k_CylinderSegments; ++i)
			{
				const float angle0 = (static_cast<float>(i) / k_CylinderSegments) * k_TwoPi;
				const float angle1 = (static_cast<float>(i + 1) / k_CylinderSegments) * k_TwoPi;

				const float x0 = std::cos(angle0) * radius, y0 = std::sin(angle0) * radius;
				const float x1 = std::cos(angle1) * radius, y1 = std::sin(angle1) * radius;

				const Vec3 bottomLeft = world(x0, y0, bottom);
				const Vec3 bottomRight = world(x1, y1, bottom);
				const Vec3 topLeft = world(x0, y0, top);
				const Vec3 topRight = world(x1, y1, top);

				push(bottomLeft); push(bottomRight); push(topRight);
				push(bottomLeft); push(topRight); push(topLeft);

				push(world(0.0f, 0.0f, top)); push(topLeft); push(topRight);

				push(world(0.0f, 0.0f, bottom)); push(bottomRight); push(bottomLeft);
			}
		}
		else if (zone.Kind == ZoneKind::Box)
		{
			const float halfWidth = zone.Radius * 0.5f;
			const float halfLength = zone.Length * 0.5f;

			const std::array<Vec3, 8> corner = {
				world(-halfWidth, -halfLength, bottom), world(halfWidth, -halfLength, bottom),
				world(halfWidth, halfLength, bottom), world(-halfWidth, halfLength, bottom),
				world(-halfWidth, -halfLength, top), world(halfWidth, -halfLength, top),
				world(halfWidth, halfLength, top), world(-halfWidth, halfLength, top)
			};

			auto quad = [&](int a, int b, int c, int d) {
				push(corner[a]); push(corner[b]); push(corner[c]);
				push(corner[a]); push(corner[c]); push(corner[d]);
				};

			quad(0, 1, 2, 3);
			quad(4, 5, 6, 7);
			quad(0, 1, 5, 4);
			quad(2, 3, 7, 6);
			quad(1, 2, 6, 5);
			quad(3, 0, 4, 7);
		}
	}
}