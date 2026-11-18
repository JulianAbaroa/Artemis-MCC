module;

#include <d3d11.h>
#include <dxgi.h>

export module Engine.Render.Type;

import std;

export namespace Engine::Render::Type
{
    using PresentFunction = auto(__stdcall*)(IDXGISwapChain* swapChain,
        UINT syncInterval, UINT flags) -> HRESULT;

    using ResizeBuffersFunction = auto(__stdcall*)(IDXGISwapChain* swapChain,
        UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat,
        UINT swapChainFlags) -> HRESULT;

    // A HUD pass of the game (chud): void(uint32 user).
    using ChudDrawFunction = auto(__fastcall*)(std::uint32_t user) -> void;

    // What the HUD detour measured at the last call of the draw for user 0 (VW.6.x). Only reads, never drawn from.
    struct HudHookInfo
    {
        bool IsHooked{ false };         // the draw detour is installed
        bool HasSample{ false };        // the draw ran at least once
        bool IsSameThread{ false };     // it ran on the thread of Present. When false the next fields are not read
        bool HasTarget{ false };        // a render target was bound in slot 0
        bool IsBackBuffer{ false };     // that target is the back buffer of the swap chain
        bool HasViewport{ false };
        std::uint32_t CallsPerFrame{ 0 };   // calls for user 0 between the last two Present
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        std::uint32_t Format{ 0 };      // DXGI_FORMAT of the target view
        float ViewportX{ 0.0f }, ViewportY{ 0.0f }, ViewportWidth{ 0.0f }, ViewportHeight{ 0.0f };
    };

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
        Opaque,         // depth test and depth write (the depth is reversed: near 1, far 0, the test is greater or equal)
        Overlay,        // depth test without depth write
        Translucent,    // depth test without depth write, blended by alpha
        DepthOnly,      // depth test and depth write, no color written, pushed away from the camera so the wireframe of the same mesh wins
        Wireframe       // depth test without depth write, wireframe lines with no bias
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

    // Longest wait of a render hook for the frame in flight when it is removed.
    inline constexpr std::chrono::milliseconds k_IdleTimeout{ 1000 };
}