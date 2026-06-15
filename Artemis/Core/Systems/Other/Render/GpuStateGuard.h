#pragma once

#include <d3d11.h>

class GpuStateGuard
{
public:
    explicit GpuStateGuard(ID3D11DeviceContext* ctx);
    ~GpuStateGuard();

    GpuStateGuard(const GpuStateGuard&) = delete;
    GpuStateGuard& operator=(const GpuStateGuard&) = delete;

    void Restore();

private:
    ID3D11DeviceContext* m_Ctx = nullptr;

    ID3D11InputLayout* m_InputLayout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY m_Topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

    ID3D11Buffer* m_VertexBuffer = nullptr;
    UINT m_VertexStride = 0;
    UINT m_VertexOffset = 0;

    ID3D11Buffer* m_IndexBuffer = nullptr;
    DXGI_FORMAT m_IndexFormat = DXGI_FORMAT_UNKNOWN;
    UINT m_IndexOffset = 0;

    ID3D11VertexShader* m_VS = nullptr;
    ID3D11Buffer* m_VSConstantBuffer = nullptr;

    ID3D11PixelShader* m_PS = nullptr;
    ID3D11Buffer* m_PSConstantBuffer = nullptr;

    ID3D11RasterizerState* m_RasterState = nullptr;
    UINT m_NumViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT m_Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};

    ID3D11RenderTargetView* m_RTV = nullptr;
    ID3D11DepthStencilView* m_DSV = nullptr;
    ID3D11DepthStencilState* m_DepthStencilState = nullptr;
    UINT m_StencilRef = 0;
    ID3D11BlendState* m_BlendState = nullptr;
    float m_BlendFactor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
    UINT m_SampleMask = 0xffffffff;

    bool m_Restored = false;
};