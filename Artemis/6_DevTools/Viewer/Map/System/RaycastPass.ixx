module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:RaycastPass;

import Common.Log.System;
import Engine.Render.System;
import Views.Raycast.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Map.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws the raycasts of the aim and the perception as lines. Colors depend on what each ray hit.
    class RaycastPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using Raycasts = Views::Raycast::Type::Raycasts;

    public:
        explicit RaycastPass(Log& log) : m_Log(log) {}
        ~RaycastPass() = default;

        RaycastPass(const RaycastPass&) = delete;
        auto operator=(const RaycastPass&) -> RaycastPass& = delete;

        // Rebuilds the vertices when the generation changes.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Raycasts>& raycasts, std::uint64_t generation) -> void;

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