module;

#include <d3d11.h>
#include <wrl/client.h>

export module DevTools.Viewer.Map.System:MapPass;

import Common.Log.System;
import Engine.Render.Type;
import Engine.Render.System;
import Blam.MapModel.Definitions.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Map.Type;
import std;

export namespace DevTools::Viewer::Map::System
{
    // Draws the static map geometry, either the render geometry or the collision the rays hit. Both vertex buffers are built once.
    class MapPass
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using Log = Common::Log::System::Log;

        using GpuPipeline = Engine::Render::System::GpuPipeline;

        using Sbsp = Blam::MapModel::Definitions::Type::Sbsp::Sbsp;

    public:
        explicit MapPass(Log& log) : m_Log(log) {}
        ~MapPass() = default;

        MapPass(const MapPass&) = delete;
        auto operator=(const MapPass&) -> MapPass& = delete;

        // Builds and uploads the vertex buffer. Does nothing if it was already uploaded.
        // note: The map counts as uploaded even if it failed, so it is not retried until Release.
        auto Upload(ID3D11Device* device, const std::unordered_map<std::string, Sbsp>& sbsps) -> void;

        auto IsUploaded() const -> bool;

        // return: True if there is geometry to draw.
        auto HasBuffer() const -> bool;

        // param isCollision: Draws the collision surfaces instead of the render geometry. Falls back to the other one if it is empty.
        auto Draw(ID3D11DeviceContext* context, GpuPipeline& pipeline, bool isCollision) -> void;

        auto Release() -> void;

    private:
        Log& m_Log;

        ComPtr<ID3D11Buffer> m_VertexBuffer{};
        UINT m_VertexCount{ 0 };
        ComPtr<ID3D11Buffer> m_CollisionBuffer{};
        UINT m_CollisionCount{ 0 };

        // Uploads the vertices as an immutable buffer.
        // return: False if they do not fit in one buffer or the device refuses.
        auto CreateBuffer(ID3D11Device* device, const std::vector<Engine::Render::Type::Vertex>& vertices,
            ComPtr<ID3D11Buffer>& outBuffer, UINT& outCount) -> bool;
        bool m_IsUploaded{ false };
    };
}