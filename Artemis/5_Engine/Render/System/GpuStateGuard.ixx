module;

#include <d3d11.h>
#include <wrl/client.h>

export module Engine.Render.System:GpuStateGuard;

import std;

export namespace Engine::Render::System
{
    // Saves the pipeline state of the context when created and restores it when destroyed.
    // note: Saves the render targets 0 to 7 with the depth stencil and every stage Artemis draws with: input assembler, shaders, vertex constant
    // buffer 0, rasterizer with viewports and scissors, depth stencil, blend and predication (which is off while the guard lives). Shader resources and samplers are not saved: the passes bind none.
    class GpuStateGuard
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        explicit GpuStateGuard(ID3D11DeviceContext* context);
        ~GpuStateGuard();

        GpuStateGuard(const GpuStateGuard&) = delete;
        auto operator=(const GpuStateGuard&) -> GpuStateGuard& = delete;

    private:
        static constexpr UINT k_VertexSlots{ 2 };
        static constexpr UINT k_TargetSlots{ D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT };

        ID3D11DeviceContext* m_Context{ nullptr };

        ComPtr<ID3D11InputLayout> m_InputLayout{};
        D3D11_PRIMITIVE_TOPOLOGY m_Topology{ D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED };

        std::array<ComPtr<ID3D11Buffer>, k_VertexSlots> m_VertexBuffers{};
        std::array<UINT, k_VertexSlots> m_VertexStrides{};
        std::array<UINT, k_VertexSlots> m_VertexOffsets{};

        ComPtr<ID3D11Buffer> m_IndexBuffer{};
        DXGI_FORMAT m_IndexFormat{ DXGI_FORMAT_UNKNOWN };
        UINT m_IndexOffset{ 0 };

        ComPtr<ID3D11VertexShader> m_VertexShader{};
        ComPtr<ID3D11Buffer> m_VertexConstantBuffer{};
        ComPtr<ID3D11HullShader> m_HullShader{};
        ComPtr<ID3D11DomainShader> m_DomainShader{};
        ComPtr<ID3D11GeometryShader> m_GeometryShader{};
        ComPtr<ID3D11PixelShader> m_PixelShader{};

        ComPtr<ID3D11RasterizerState> m_Rasterizer{};

        ComPtr<ID3D11Predicate> m_Predicate{};
        BOOL m_PredicateValue{ FALSE };
        UINT m_ViewportCount{ 0 };
        std::array<D3D11_VIEWPORT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> m_Viewports{};

        UINT m_ScissorCount{ 0 };
        std::array<D3D11_RECT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> m_Scissors{};

        std::array<ComPtr<ID3D11RenderTargetView>, k_TargetSlots> m_RenderTargets{};
        ComPtr<ID3D11DepthStencilView> m_DepthStencil{};
        ComPtr<ID3D11DepthStencilState> m_DepthState{};
        UINT m_StencilRef{ 0 };
        ComPtr<ID3D11BlendState> m_BlendState{};
        std::array<float, 4> m_BlendFactor{ 0.0f, 0.0f, 0.0f, 0.0f };
        UINT m_SampleMask{ 0xffffffff };
    };
}