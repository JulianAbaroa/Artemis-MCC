module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:ObsPass;

import Common.Log.System;
import Common.Math.Type;
import Engine.Render.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Interpolation.Type;
import DevTools.Viewer.Map.Type;
import DevTools.Viewer.Options.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws, for each Artemis agent, the two arrows its policy receives in its observation (VW.17): from its feet to the enemy flag, in the color of the
    // rival team, and to its own stand, in the color of its team. The same View row and the same conditions as ObserveBatch (ObsTargetsOf).
    // note: The arrows start at the biped as the overlay draws it (the interpolation offset, VW.5e) and end where the View sees the flag and the stand.
    class ObsPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using Tick = Bridge::Codec::Tick::Type::Tick;
        using FramePoses = DevTools::Viewer::Interpolation::Type::FramePoses;
        using Level = DevTools::Viewer::Options::Type::Level;

    public:
        explicit ObsPass(Log& log) : m_Log(log) {}
        ~ObsPass() = default;

        ObsPass(const ObsPass&) = delete;
        auto operator=(const ObsPass&) -> ObsPass& = delete;

        // Rebuilds the vertices when the generation, the level of the layer or the poses of the frame change.
        // param tick: The tick drawn, with the Views. Null draws nothing.
        // param agentRows: The rows of Players (and of the ViewTable) that are agents.
        // param level: The level of the layer. Off draws nothing.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context, const Tick* tick, const std::vector<std::size_t>& agentRows,
            Level level, const FramePoses& poses) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        Log& m_Log;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_Capacity{ 0 };
        UINT m_VertexCount{ 0 };

        Engine::Render::System::UploadGate m_Gate{};
    };
}