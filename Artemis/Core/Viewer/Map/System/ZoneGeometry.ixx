export module Viewer.Map.System:ZoneGeometry;

import Platform.Render.System;
import Common.Math.Type;
import Common.ZoneShape.Type;
import Viewer.Map.Type;
import std;

export namespace Viewer::Map::System::ZoneGeometry
{
	using Vertex = Platform::Render::System::GpuPipeline::Vertex;
	using Vec3 = Common::Math::Type::Vec3;
	using ZoneShape = Common::ZoneShape::Type::ZoneShape;
	using Color = Viewer::Map::Type::Color;

	auto AppendSolid(std::vector<Vertex>& out, const Vec3& position,
		const Vec3& forward, const Vec3& up, const ZoneShape& zone,
		const Color& color) -> void;
}