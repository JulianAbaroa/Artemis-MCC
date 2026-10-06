module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:MapPass;

import Service.Logs.System;
import Platform.Render.System;
import Export.Tick.Type;
import std;

export namespace Viewer::Map::System
{
    // Draws the static map geometry. The vertex buffer is built once from the render geometry of the map.
    class MapPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using LogsService = Service::Logs::System::LogsService;

        using GpuPipeline = Platform::Render::System::GpuPipeline;

        using MapSbsps = Export::Tick::Type::MapSbsps;

    public:
        explicit MapPass(LogsService& logsService) : m_LogsService(logsService) {}
        ~MapPass() = default;

        MapPass(const MapPass&) = delete;
        auto operator=(const MapPass&) -> MapPass& = delete;

        // Builds and uploads the vertex buffer. Does nothing if it was already uploaded.
        // note: The map counts as uploaded even if it failed, so it is not retried until Release.
        auto Upload(ID3D11Device* device, const MapSbsps& sbsps) -> void;

        auto IsUploaded() const -> bool;

        // return: True if there is geometry to draw.
        auto HasBuffer() const -> bool;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        LogsService& m_LogsService;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_VertexCount{ 0 };
        bool m_IsUploaded{ false };
    };
}