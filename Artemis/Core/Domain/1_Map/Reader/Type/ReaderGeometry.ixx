export module Map.Reader.Type:Geometry;

import Common.Math.Type;
import std;

namespace
{
	using Triangle = Common::Math::Type::Triangle;
}

export namespace Map::Reader::Type::Geometry
{
	struct BufferInfo
	{
		std::uint32_t DataLength{};
		std::uint32_t Aux{}; // Vertex Count or Index Format.
	};

	struct SbspGeometry
	{
		std::string TagName{};
		std::vector<Triangle> RenderGeometry{};
	};

	struct VertexDecodeContext
	{
		const std::uint8_t* Buffer{ nullptr };
		std::uint32_t Count{ 0 };
		std::uint32_t Stride{ 0 };

		bool HasBounds{ false };

		float MinX{ 0 };
		float MinY{ 0 };
		float MinZ{ 0 };

		float LengthX{ 0 };
		float LengthY{ 0 };
		float LengthZ{ 0 };

		const float* TransformMatrix{ nullptr };
	};
}