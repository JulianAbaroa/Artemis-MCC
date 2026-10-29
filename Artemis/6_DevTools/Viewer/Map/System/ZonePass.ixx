module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:ZonePass;

import Common.Log.System;
import Engine.Render.Type;
import Engine.Render.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Palette.System;
import DevTools.Viewer.Map.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws the zones of the teleporters and the objective spawns as translucent solids.
    class ZonePass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using Vertex = Engine::Render::Type::Vertex;
        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using Tick = Bridge::Codec::Tick::Type::Tick;

        using PaletteService = DevTools::Viewer::Palette::System::PaletteService;

    public:
        explicit ZonePass(Log& log) : m_Log(log) {}
        ~ZonePass() = default;

        ZonePass(const ZonePass&) = delete;
        auto operator=(const ZonePass&) -> ZonePass& = delete;

        // Rebuilds the vertices when the generation of the tick changes. The color of each zone comes from the palette.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Tick>& tick, const PaletteService& palette) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        Log& m_Log;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_Capacity{ 0 };
        UINT m_VertexCount{ 0 };

        Engine::Render::System::UploadGate m_Gate{};

        std::vector<Vertex> m_Scratch{};
    };
}