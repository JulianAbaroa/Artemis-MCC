module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.Type;

import Platform.Render.Type;
import std;

export namespace Viewer::Map::Type
{
    using MeshInstance = Platform::Render::Type::MeshInstance;

    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // Which fixture arrows the fixture pass draws, and how long the direction arrows are in world units.
    struct FixturePassOptions
    {
        bool TeleportLinks{ true };
        bool LiftArrows{ true };
        bool ShieldArrows{ true };
        float ArrowLength{ 3.0f };
    };

    // Which map limits the limits pass draws and how opaque they are.
    struct LimitsPassOptions
    {
        bool KillVolumes{ true };
        bool SafeVolumes{ true };
        bool SoftCeilings{ true };
        float Opacity{ 0.20f };
    };

    // Vertices of one kind of primitive and the dynamic buffer that holds them.
    struct VertexBatch
    {
        ComPtr<ID3D11Buffer> Buffer{};
        UINT Capacity{ 0 };
        UINT VertexCount{ 0 };
        std::vector<Platform::Render::Type::Vertex> Scratch{};
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