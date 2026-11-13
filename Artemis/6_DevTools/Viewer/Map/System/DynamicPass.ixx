module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:DynamicPass;

import Common.Log.System;
import Engine.Render.Type;
import Engine.Render.System;
import Perception.Collidable.Type;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Palette.System;
import DevTools.Viewer.Map.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws the collidable meshes with GPU instancing. Each mesh is stored once and drawn once per instance.
    // The selected object is drawn in the selection color, and the translucent handles go to a second draw.
    class DynamicPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using MeshInstance = Engine::Render::Type::MeshInstance;
        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using Collidables = Perception::Collidable::Type::Collidables;

        using PaletteService = DevTools::Viewer::Palette::System::PaletteService;
        using Geometry = DevTools::Viewer::Map::Type::Geometry;
        using DrawRange = DevTools::Viewer::Map::Type::DrawRange;

    public:
        explicit DynamicPass(Log& log) : m_Log(log) {}
        ~DynamicPass() = default;

        DynamicPass(const DynamicPass&) = delete;
        auto operator=(const DynamicPass&) -> DynamicPass& = delete;

        // Rebuilds the instances when a new scene is published or the selection or the translucent set change.
        // param translucentHandles: Objects drawn by DrawTranslucent instead of Draw.
        // note: The scene is published after the Tick (D223), so it is compared by snapshot and not by the Tick generation. A gate on
        // the generation drew the previous scene for a whole tick whenever the frame saw the Tick first (D294).
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Collidables>& collidables,
            const PaletteService& palette, std::uint32_t selectedHandle,
            const std::unordered_set<std::uint32_t>& translucentHandles) -> void;

        // return: Instances uploaded in the last rebuild, translucent ones included.
        auto GetInstanceCount() const -> UINT;

        // return: Draw calls of the opaque pass.
        auto GetDrawCount() const -> UINT;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        // param alpha: Opacity of the translucent objects.
        auto DrawTranslucent(ID3D11DeviceContext* context, GpuPipeline& pipeline, float alpha) -> void;

        auto Release() -> void;

    private:
        Log& m_Log;

        ComPtr<ID3D11Buffer> m_GeometryBuffer{};
        UINT m_GeometryCapacity{ 0 };
        UINT m_GeometryUploaded{ 0 };
        UINT m_GeometryTotal{ 0 };
        std::vector<float> m_PendingVertices{};

        std::unordered_map<const void*, std::uint32_t> m_GeometryIndex{};
        std::vector<Geometry> m_Geometries{};
        std::vector<std::uint32_t> m_Active{};
        std::vector<std::uint32_t> m_ActiveTranslucent{};

        Engine::Render::System::InstanceBuffer m_InstanceBuffer{};
        UINT m_InstanceCount{ 0 };

        std::vector<MeshInstance> m_InstanceScratch{};
        std::vector<DrawRange> m_Draws{};
        std::vector<DrawRange> m_TranslucentDraws{};

        // Binds the geometry and instance buffers and draws the ranges.
        auto DrawRanges(ID3D11DeviceContext* context, const std::vector<DrawRange>& ranges) const -> void;

        // Scene of the last rebuild. Kept alive so a newer snapshot can never reuse its address.
        std::shared_ptr<const Collidables> m_UploadedScene{};
        std::uint32_t m_UploadedKey{ 0 };
        bool m_IsUploaded{ false };

        // Copies the pending vertices to the geometry buffer and grows it if needed.
        // return: False if the buffer could not be created.
        auto FlushGeometry(ID3D11Device* device, ID3D11DeviceContext* context) -> bool;

        auto UploadInstances(ID3D11Device* device, ID3D11DeviceContext* context) -> void;
    };
}