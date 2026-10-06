module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:DynamicPass;

import Service.Logs.System;
import Platform.Render.Type;
import Platform.Render.System;
import Export.Tick.Type;
import Viewer.Palette.System;
import Viewer.Map.Type;
import std;

export namespace Viewer::Map::System
{
    // Draws the collidable meshes with GPU instancing. Each mesh is stored once and drawn once per instance.
    // The selected object is drawn in the selection color, and the translucent handles go to a second draw.
    class DynamicPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using LogsService = Service::Logs::System::LogsService;

        using MeshInstance = Platform::Render::Type::MeshInstance;
        using GpuPipeline = Platform::Render::System::GpuPipeline;

        using Collidables = Export::Tick::Type::Collidables;

        using PaletteService = Viewer::Palette::System::PaletteService;
        using Geometry = Viewer::Map::Type::Geometry;
        using DrawRange = Viewer::Map::Type::DrawRange;

    public:
        explicit DynamicPass(LogsService& logsService) : m_LogsService(logsService) {}
        ~DynamicPass() = default;

        DynamicPass(const DynamicPass&) = delete;
        auto operator=(const DynamicPass&) -> DynamicPass& = delete;

        // Rebuilds the instances when the tick generation, the selection or the translucent set change.
        // param translucentHandles: Objects drawn by DrawTranslucent instead of Draw.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Collidables>& collidables,
            const PaletteService& palette, std::uint32_t selectedHandle,
            const std::unordered_set<std::uint32_t>& translucentHandles,
            std::uint64_t generation) -> void;

        // return: Instances uploaded in the last rebuild, translucent ones included.
        auto GetInstanceCount() const -> UINT;

        // return: Draw calls of the opaque pass.
        auto GetDrawCount() const -> UINT;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        // param alpha: Opacity of the translucent objects.
        auto DrawTranslucent(ID3D11DeviceContext* context, GpuPipeline& pipeline, float alpha) -> void;

        auto Release() -> void;

    private:
        LogsService& m_LogsService;

        ComPtr<ID3D11Buffer> m_GeometryBuffer{};
        UINT m_GeometryCapacity{ 0 };
        UINT m_GeometryUploaded{ 0 };
        UINT m_GeometryTotal{ 0 };
        std::vector<float> m_PendingVertices{};

        std::unordered_map<const void*, std::uint32_t> m_GeometryIndex{};
        std::vector<Geometry> m_Geometries{};
        std::vector<std::uint32_t> m_Active{};
        std::vector<std::uint32_t> m_ActiveTranslucent{};

        Platform::Render::System::GpuBuffer::InstanceBuffer m_InstanceBuffer{};
        UINT m_InstanceCount{ 0 };

        std::vector<MeshInstance> m_InstanceScratch{};
        std::vector<DrawRange> m_Draws{};
        std::vector<DrawRange> m_TranslucentDraws{};

        Platform::Render::System::GpuBuffer::UploadGate m_Gate{};

        // Copies the pending vertices to the geometry buffer and grows it if needed.
        // return: False if the buffer could not be created.
        auto FlushGeometry(ID3D11Device* device, ID3D11DeviceContext* context) -> bool;

        auto UploadInstances(ID3D11Device* device, ID3D11DeviceContext* context) -> void;
    };
}