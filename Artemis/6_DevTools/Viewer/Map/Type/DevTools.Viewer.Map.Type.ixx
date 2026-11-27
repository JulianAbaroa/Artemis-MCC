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

    // World units. How far above the feet the passes of the View and of the observation lift what they draw for a player (a biped is about 0.7 tall, so
    // this is about its middle).
    inline constexpr float k_MarkerLift{ 0.35f };

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

    // Folds the rows a pass draws and a revision of the poses (or of the level) into a value for its upload gate (VW.17a). Another list of rows, or other
    // poses, upload again.
    inline auto ShownRowsKeyOf(const std::vector<std::size_t>& rows, std::uint32_t revision) -> std::uint32_t
    {
        constexpr std::uint32_t k_RowHash{ 0x9E3779B1u };

        std::uint32_t key{ revision };
        for (const std::size_t row : rows) key = (key ^ static_cast<std::uint32_t>(row + 1)) * k_RowHash;
        return key ^ static_cast<std::uint32_t>(rows.size());
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
        std::vector<MeshInstance> DepthBucket{};
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