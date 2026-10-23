module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:FixturePass;

import Common.Log.System;
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
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using GpuPipeline = Platform::Render::System::GpuPipeline;

        using Tick = Export::Tick::Type::Tick;

        using FixturePassOptions = Viewer::Map::Type::FixturePassOptions;

    public:
        explicit FixturePass(Log& log) : m_Log(log) {}
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
        Log& m_Log;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_Capacity{ 0 };
        UINT m_VertexCount{ 0 };
    };
}