module;

#include <d3d11.h>

export module Viewer.Map.System:FixturePass;

import Service.Logs.System;
import Platform.Render.System;
import Export.Tick.Type;
import Viewer.Map.Type;
import std;

export namespace Viewer::Map::System
{
    // Draws the arrows of the map logic: teleporter links, lift launch directions and shield block directions.
    class FixturePass
    {
    private:
        template <typename T>
        using ComPtr = Viewer::Map::Type::ComPtr<T>;

        using LogsService = Service::Logs::System::LogsService;

        using GpuPipeline = Platform::Render::System::GpuPipeline;

        using Tick = Export::Tick::Type::Tick;

        using FixturePassOptions = Viewer::Map::Type::FixturePassOptions;

    public:
        explicit FixturePass(LogsService& logsService) : m_LogsService(logsService) {}
        ~FixturePass() = default;

        FixturePass(const FixturePass&) = delete;
        auto operator=(const FixturePass&) -> FixturePass& = delete;

        // Rebuilds the arrows from the tick. Draws nothing if the tick is missing.
        // note: Every arrow comes from the Details and the Entities: teleporter links (D90), lift launches and barrier pass directions (D91).
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Tick>& tick, const FixturePassOptions& options) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        LogsService& m_LogsService;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_Capacity{ 0 };
        UINT m_VertexCount{ 0 };
    };
}