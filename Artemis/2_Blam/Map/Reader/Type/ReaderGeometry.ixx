export module Blam.Map.Reader.Type:Geometry;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Triangle;
}

export namespace Blam::Map::Reader::Type::Geometry
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

    // One placement of an instanced geometry definition.
    struct CollisionInstance
    {
        std::int16_t DefinitionIndex{ -1 };
        std::uint16_t Flags{};

        // Three rows of the rotation already multiplied by the scale, then the position. Row vectors, as the render geometry.
        std::array<float, 12> Matrix{};
    };

    // Collision of one structure BSP.
    struct SbspCollision
    {
        std::string TagName{};

        // Surfaces of the structure as triangles, in world space.
        std::vector<Triangle> Structure{};

        // Surfaces of each instanced geometry definition as triangles, in the local space of the definition.
        std::vector<std::vector<Triangle>> Definitions{};
        std::vector<CollisionInstance> Instances{};

        // True if the structure used the large element layout.
        bool IsLarge{ false };

        // Surfaces whose polygon could not be walked, triangles dropped for having no area, definitions that could not be
        // read and placements that name a definition that does not exist.
        std::uint32_t OpenSurfaces{};
        std::uint32_t DegenerateTriangles{};
        std::uint32_t UnreadDefinitions{};
        std::uint32_t InvalidPlacements{};
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