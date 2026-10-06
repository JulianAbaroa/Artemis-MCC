module;

#include <d3d11.h>
#include <dxgi.h>

export module Platform.Render.Type;

import std;

export namespace Platform::Render::Type
{
    using PresentFunction = auto(__stdcall*)(IDXGISwapChain* swapChain,
        UINT syncInterval, UINT flags) -> HRESULT;

    using ResizeBuffersFunction = auto(__stdcall*)(IDXGISwapChain* swapChain,
        UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat,
        UINT swapChainFlags) -> HRESULT;

    // Layout of the vertex data of a draw. Each one has its own input layout and vertex shader.
    enum class VertexLayout : std::uint8_t
    {
        Colored,            // Vertex per vertex
        SphereInstanced,    // unit mesh positions plus a SphereInstance per instance
        MeshInstanced       // unit mesh positions plus a MeshInstance per instance
    };

    // How a draw is blended and depth tested.
    enum class SurfaceMode : std::uint8_t
    {
        Opaque,         // depth test and depth write
        Overlay,        // depth test without depth write
        Translucent     // depth test without depth write, blended by alpha
    };

    struct Vertex
    {
        float X{}, Y{}, Z{};
        float R{}, G{}, B{};
    };

    // Sphere drawn by instancing a unit sphere.
    struct SphereInstance
    {
        float X{}, Y{}, Z{}, Radius{};  // center and radius
        float R{}, G{}, B{};
    };

    // Mesh drawn by instancing a unit mesh.
    struct MeshInstance
    {
        std::array<float, 12> Transform{};  // 3 rows of 4 floats, the last row is implicit
        float R{}, G{}, B{};
        float Padding{};                    // pads the instance to 64 bytes
    };

    // Addresses of the IDXGISwapChain methods to hook. Null when not located.
    struct SwapChainAddresses
    {
        void* Present{ nullptr };
        void* ResizeBuffers{ nullptr };
    };

    // What the handlers receive on each render event.
    struct FrameContext
    {
        ID3D11Device* Device{ nullptr };
        ID3D11DeviceContext* Context{ nullptr };
        ID3D11RenderTargetView* RenderTarget{ nullptr };
        ID3D11DepthStencilView* DepthStencil{ nullptr };  // may be null if its creation failed
        HWND Window{ nullptr };

        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
    };

    using FrameHandler = std::function<void(const FrameContext&)>;
    using ShutdownCallback = std::function<void()>;
}