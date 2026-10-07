export module Map.Reader.Type:Geometry;

import Common.Math.Type;
import std;

namespace
{
    using Triangle = Common::Math::Type::Triangle;
}

export namespace Map::Reader::Type::Geometry
{
    // Size and auxiliary value of a vertex or index buffer, read from the buffer info tables.
    struct BufferInfo
    {
        std::uint32_t DataLength{};

        // Vertex count for a vertex buffer, index format for an index buffer.
        std::uint32_t Aux{};
    };

    // Render geometry of one structure BSP.
    struct SbspGeometry
    {
        std::string TagName{};
        std::vector<Triangle> RenderGeometry{};
    };

    // Everything needed to decode the vertices of one mesh section.
    struct VertexDecodeContext
    {
        const std::uint8_t* Buffer{ nullptr };
        std::uint32_t Count{ 0 };
        std::uint32_t Stride{ 0 };

        // Positions are stored normalized and need the min and length of each axis to be restored.
        bool HasBounds{ false };

        float MinX{ 0 };
        float MinY{ 0 };
        float MinZ{ 0 };

        float LengthX{ 0 };
        float LengthY{ 0 };
        float LengthZ{ 0 };

        // Optional 3x4 matrix that moves the vertices to world space. Null if the vertices are already there.
        const float* TransformMatrix{ nullptr };
    };
}