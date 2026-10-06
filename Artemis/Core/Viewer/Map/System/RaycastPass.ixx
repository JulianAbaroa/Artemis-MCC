module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:RaycastPass;

import Service.Logs.System;
import Export.Tick.Type;
import Platform.Render.System;
import std;

export namespace Viewer::Map::System
{
    // Draws the raycasts of the aim and the perception as lines. Colors depend on what each ray hit.
    class RaycastPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using LogsService = Service::Logs::System::LogsService;

        using GpuPipeline = Platform::Render::System::GpuPipeline;

        using Raycasts = Export::Tick::Type::Raycasts;

    public:
        explicit RaycastPass(LogsService& logsService) : m_LogsService(logsService) {}
        ~RaycastPass() = default;

        RaycastPass(const RaycastPass&) = delete;
        auto operator=(const RaycastPass&) -> RaycastPass& = delete;

        // Rebuilds the vertices when the generation changes.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const std::shared_ptr<const Raycasts>& raycasts, std::uint64_t generation) -> void;

        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline) -> void;

        auto Release() -> void;

    private:
        LogsService& m_LogsService;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_Capacity{ 0 };
        UINT m_VertexCount{ 0 };

        Platform::Render::System::GpuBuffer::UploadGate m_Gate{};
    };
}