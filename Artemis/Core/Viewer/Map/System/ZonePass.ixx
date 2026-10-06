module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:ZonePass;

import Service.Logs.System;
import Platform.Render.Type;
import Platform.Render.System;
import Export.Tick.Type;
import Viewer.Palette.System;
import std;

export namespace Viewer::Map::System
{
    // Draws the zones of the teleporters and the objective spawns as translucent solids.
    class ZonePass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using LogsService = Service::Logs::System::LogsService;

        using Vertex = Platform::Render::Type::Vertex;
        using GpuPipeline = Platform::Render::System::GpuPipeline;

        using Tick = Export::Tick::Type::Tick;

        using PaletteService = Viewer::Palette::System::PaletteService;

    public:
        explicit ZonePass(LogsService& logsService) : m_LogsService(logsService) {}
        ~ZonePass() = default;

        ZonePass(const ZonePass&) = delete;
        auto operator=(const ZonePass&) -> ZonePass& = delete;

        // Rebuilds the vertices when the generation of the tick changes. The color of each zone comes from the palette.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Tick>& tick, const PaletteService& palette) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        LogsService& m_LogsService;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_Capacity{ 0 };
        UINT m_VertexCount{ 0 };

        Platform::Render::System::GpuBuffer::UploadGate m_Gate{};

        std::vector<Vertex> m_Scratch{};
    };
}