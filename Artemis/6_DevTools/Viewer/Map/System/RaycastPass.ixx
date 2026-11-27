module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:RaycastPass;

import Common.Log.System;
import Engine.Render.System;
import Views.View.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Interpolation.Type;
import DevTools.Viewer.Map.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws the raycasts of the aim and the perception of each shown View as lines. Colors depend on what each ray hit.
    class RaycastPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using ViewTable = Views::View::Type::ViewTable;
        using FramePoses = DevTools::Viewer::Interpolation::Type::FramePoses;

    public:
        explicit RaycastPass(Log& log) : m_Log(log) {}
        ~RaycastPass() = default;

        RaycastPass(const RaycastPass&) = delete;
        auto operator=(const RaycastPass&) -> RaycastPass& = delete;

        // Rebuilds the vertices when the generation, the rows or the poses change.
        // param views: The Views of the tick. Null draws nothing.
        // param rows: The rows of the Views drawn (VW.17a). A row whose View is not computed draws nothing.
        // param poses: The poses of the frame (VW.5e). The rays of each View start from its origin moved by the offset of its biped and end where the
        // scene hit.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const ViewTable* views, const std::vector<std::size_t>& rows, std::uint64_t generation, const FramePoses& poses) -> void;

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