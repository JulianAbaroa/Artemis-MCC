module;

#include <d3d11.h>
#include <wrl/client.h>

export module Platform.Render.System:GpuPipeline;

import Service.Logs.System;
import Platform.Render.Type;
import std;

export namespace Platform::Render::System
{
    // Shaders, input layouts and fixed states of the map drawing.
    // Initialize it once, call UpdateCamera and Bind before each draw, and Release it on shutdown.
    class GpuPipeline
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using LogsService = Service::Logs::System::LogsService;

        using Vertex = Platform::Render::Type::Vertex;
        using SphereInstance = Platform::Render::Type::SphereInstance;
        using MeshInstance = Platform::Render::Type::MeshInstance;
        using VertexLayout = Platform::Render::Type::VertexLayout;
        using SurfaceMode = Platform::Render::Type::SurfaceMode;

        using Matrix = std::array<float, 16>;

    public:
        // Strides of the vertex buffers.
        static constexpr UINT k_VertexStride{ sizeof(Vertex) };                 // Colored
        static constexpr UINT k_UnitVertexStride{ 3 * sizeof(float) };          // position only, slot 0 of the instanced layouts
        static constexpr UINT k_InstanceStride{ sizeof(SphereInstance) };       // SphereInstanced, slot 1
        static constexpr UINT k_MeshInstanceStride{ sizeof(MeshInstance) };     // MeshInstanced, slot 1

        explicit GpuPipeline(LogsService& logsService) : m_LogsService(logsService) {}
        ~GpuPipeline() = default;

        GpuPipeline(const GpuPipeline&) = delete;
        auto operator=(const GpuPipeline&) -> GpuPipeline& = delete;

        // Compiles the shaders and creates the buffers and states. Does nothing if already ready.
        // return: False if anything fails. Everything created is released.
        auto Initialize(ID3D11Device* device) -> bool;

        // Uploads the view projection matrix to the camera constant buffer.
        auto UpdateCamera(ID3D11DeviceContext* context, const Matrix& viewProjection) -> void;

        // Sets the shaders and the states of the layout and surface for the next draws.
        // param alpha: Opacity of the Translucent surface. Ignored by the others.
        // note: Does nothing if not ready. The caller binds the vertex buffers.
        auto Bind(ID3D11DeviceContext* context, VertexLayout layout,
            SurfaceMode surface = SurfaceMode::Opaque, float alpha = 1.0f) -> void;

        auto Release() -> void;

    private:
        LogsService& m_LogsService;

        bool m_IsReady{ false };

        ComPtr<ID3D11VertexShader> m_VertexShader{};
        ComPtr<ID3D11PixelShader> m_PixelShader{};
        ComPtr<ID3D11InputLayout> m_InputLayout{};

        ComPtr<ID3D11VertexShader> m_InstancedVertexShader{};
        ComPtr<ID3D11InputLayout> m_InstancedInputLayout{};

        ComPtr<ID3D11VertexShader> m_MeshVertexShader{};
        ComPtr<ID3D11InputLayout> m_MeshInputLayout{};

        ComPtr<ID3D11Buffer> m_CameraBuffer{};

        ComPtr<ID3D11RasterizerState> m_Rasterizer{};
        ComPtr<ID3D11DepthStencilState> m_DepthState{};
        ComPtr<ID3D11DepthStencilState> m_OverlayDepthState{};
        ComPtr<ID3D11BlendState> m_BlendState{};
        ComPtr<ID3D11BlendState> m_TranslucentBlendState{};

        auto CompileShaders(ID3D11Device* device) -> bool;
        auto CreateBuffers(ID3D11Device* device) -> bool;
        auto CreateStates(ID3D11Device* device) -> bool;
    };
}