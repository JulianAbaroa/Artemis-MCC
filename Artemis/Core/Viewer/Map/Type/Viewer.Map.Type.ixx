export module Viewer.Map.Type;

import Platform.Render.Type;
import std;

export namespace Viewer::Map::Type
{
    using MeshInstance = Platform::Render::Type::MeshInstance;

    // Which fixture arrows the fixture pass draws, and how long the direction arrows are in world units.
    struct FixturePassOptions
    {
        bool TeleportLinks{ true };
        bool LiftArrows{ true };
        bool ShieldArrows{ true };
        float ArrowLength{ 3.0f };
    };

    // A mesh stored once in the geometry buffer, with the instances to draw it with.
    // The translucent bucket holds the instances that are drawn translucent.
    struct Geometry
    {
        std::uint32_t FirstVertex{ 0 };
        std::uint32_t VertexCount{ 0 };

        std::vector<MeshInstance> Bucket{};
        std::vector<MeshInstance> TranslucentBucket{};
    };

    // One instanced draw call.
    struct DrawRange
    {
        std::uint32_t FirstVertex{ 0 };
        std::uint32_t VertexCount{ 0 };
        std::uint32_t FirstInstance{ 0 };
        std::uint32_t InstanceCount{ 0 };
    };
}