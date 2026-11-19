module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.Type;

import Common.Math.Type;
import Common.State.Type;
import Engine.Render.Type;
import DevTools.Viewer.Collidable.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::State::Type::PickKind;
    using Engine::Render::Type::MeshInstance;
    using DevTools::Viewer::Collidable::Type::CollidableScene;
    using Microsoft::WRL::ComPtr;
}

export namespace DevTools::Viewer::Map::Type
{
    // Longest the thread that draws waits for the scene of the tick the game drew to be published.
    // The cap is a choice (D329), not a measured latency: it is short because the thread that waits is the one that draws the game frame.
    // Past the cap the nearest older scene is drawn.
    inline constexpr std::chrono::microseconds k_FrameSceneWaitLimit{ 2000 };

    // The scene a frame draws and how it was chosen (VW.5d).
    struct FrameScenePick
    {
        std::shared_ptr<const CollidableScene> Scene{};
        // Exact, older or newer than the tick the game drew (T - 1). None when no target applied and the newest was taken.
        PickKind Kind{};
    };

    // Folds a set of object handles into one value for the upload gates of the passes (order independent).
    inline auto HandleSetKeyOf(const std::unordered_set<std::uint32_t>& handles) -> std::uint32_t
    {
        constexpr std::uint32_t k_HandleHash{ 0x9E3779B1u };

        std::uint32_t key{ static_cast<std::uint32_t>(handles.size()) };
        for (const std::uint32_t handle : handles) key += handle * k_HandleHash;
        return key;
    }

    // Folds an offset into a value for the upload gates of the passes: a frame with another offset is uploaded again (VW.5e). Zero for no offset.
    inline auto OffsetKeyOf(const Vec3& offset) -> std::uint32_t
    {
        constexpr std::uint32_t k_OffsetHash{ 0x9E3779B1u };

        const bool isZero{ offset.X == 0.0f && offset.Y == 0.0f && offset.Z == 0.0f };
        if (isZero) return 0u;

        return (std::bit_cast<std::uint32_t>(offset.X) * k_OffsetHash) ^ (std::bit_cast<std::uint32_t>(offset.Y) * (k_OffsetHash + 2u))
            ^ std::bit_cast<std::uint32_t>(offset.Z);
    }

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
        std::vector<Engine::Render::Type::Vertex> Scratch{};
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

    // Instances a pass buffer holds before its first growth.
    inline constexpr UINT k_InitialCapacity{ 2048 };
}