module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:RaycastPass;

import Common.Log.System;
import Common.Math.Type;
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

        using Vec3 = Common::Math::Type::Vec3;
        using Raycasts = Views::Raycast::Type::Raycasts;

    public:
        explicit RaycastPass(Log& log) : m_Log(log) {}
        ~RaycastPass() = default;

        RaycastPass(const RaycastPass&) = delete;
        auto operator=(const RaycastPass&) -> RaycastPass& = delete;

        // Rebuilds the vertices when the generation changes.
        // param raycasts: The rays of the watched player. Null for none.
        // param originOffset: How far the overlay draws the biped of the watched player from where the scene shows it (VW.5e). Every ray starts
        // from its origin moved by it and ends where the scene hit, and a frame with another offset is uploaded again.
        auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
            const Raycasts* raycasts, std::uint64_t generation, const Vec3& originOffset) -> void;

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