#pragma once

#include <d3d11.h>

class System_Logs;

class GpuPipeline
{
public:
    struct Vertex
    {
        float x, y, z;
        float r, g, b;
    };
    static constexpr UINT kVertexStride = sizeof(Vertex);

    enum class Fill { DepthPrime, Solid, Wire };

    GpuPipeline() = default;
    ~GpuPipeline() = default;

    GpuPipeline(const GpuPipeline&) = delete;
    GpuPipeline& operator=(const GpuPipeline&) = delete;

    bool Init(ID3D11Device* device, System_Logs& logs);
    bool IsReady() const { return m_Ready; }

    void UpdateCamera(ID3D11DeviceContext* ctx, const float* viewProj16);

    void Bind(ID3D11DeviceContext* ctx, Fill fill);

    void Release();

private:
    bool CompileShaders(ID3D11Device* device, System_Logs& logs);
    bool CreateStates(ID3D11Device* device, System_Logs& logs);

    void UpdateTint(ID3D11DeviceContext* ctx, Fill fill);

    bool m_Ready = false;

    ID3D11VertexShader* m_VS = nullptr;
    ID3D11PixelShader* m_PS = nullptr;
    ID3D11InputLayout* m_InputLayout = nullptr;

    ID3D11Buffer* m_CameraCB = nullptr;
    ID3D11Buffer* m_TintCB = nullptr;

    ID3D11RasterizerState* m_RasterSolid = nullptr;
    ID3D11RasterizerState* m_RasterWire = nullptr;

    ID3D11DepthStencilState* m_DepthWrite = nullptr;
    ID3D11DepthStencilState* m_DepthNoWrite = nullptr;
    ID3D11DepthStencilState* m_DepthDisabled = nullptr;

    ID3D11BlendState* m_BlendState = nullptr;
    ID3D11BlendState* m_BlendNoColor = nullptr;
    ID3D11BlendState* m_BlendOpaque = nullptr;
};