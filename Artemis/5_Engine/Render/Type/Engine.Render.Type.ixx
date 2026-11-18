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

    // The first person model pass of the game: void(void* camera, uint32 user, uint8 pass).
    using FirstPersonModelFunction = auto(__fastcall*)(void* camera, std::uint32_t user, std::uint8_t pass) -> void;

    // The game's test of a first person unit: bool(uint32 user, uint32 objectHandle).
    using IsFirstPersonUnitFunction = auto(__fastcall*)(std::uint32_t user, std::uint32_t objectHandle) -> bool;

    // Why the last completed frame did or did not draw before the HUD of the game.
    enum class BeforeHudStatus : std::uint8_t
    {
        NoHudDraw,          // the game did not run the HUD draw for user 0 in the frame
        Drawn,              // a handler drew before the HUD
        OtherThread,        // the HUD draw ran on a thread other than Present
        NotReady,           // no targets, resizing or shutting down
        TargetMismatch,     // the bound target is not a texture of the size of the back buffer
        NotBackBuffer,      // the bound target is not the current back buffer of the store
        OtherDevice,        // the bound target belongs to another device than the store's
        SplitViewport,      // viewport 0 is not the whole target (split screen)
        HudHidden,          // the game HUD is hidden (Hide game HUD)
        Declined,           // the handlers did not draw
        Failed              // a handler threw: the frame counts as handled, so Present does not draw it again
    };

    // The depth stencil view that was bound.
    struct DepthTargetInfo
    {
        bool HasDepth{ false };
        std::uint64_t Resource{ 0 };        // address of the resource, only to compare it with another sample. Never dereferenced
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        std::uint32_t ResourceFormat{ 0 };
        std::uint32_t ViewFormat{ 0 };
        std::uint32_t ViewFlags{ 0 };       // D3D11_DSV_READ_ONLY_DEPTH (1) and D3D11_DSV_READ_ONLY_STENCIL (2)
    };

    // What the HUD detour measured at the last call of the draw for user 0 (VW.6.x). Only reads, never drawn from.
    struct HudHookInfo
    {
        bool IsHooked{ false };         // the draw detour is installed
        bool HasSample{ false };        // the draw ran at least once
        bool IsSameThread{ false };     // it ran on the thread of Present. When false the next fields are not read
        bool HasTarget{ false };        // a render target was bound in slot 0
        bool IsBackBuffer{ false };     // that target is the current back buffer of the store
        bool IsSameDevice{ false };     // that target belongs to the device of the store
        bool HasViewport{ false };
        std::uint32_t CallsPerFrame{ 0 };   // calls for user 0 between the last two Present
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        std::uint32_t Format{ 0 };      // DXGI_FORMAT of the target view
        float ViewportX{ 0.0f }, ViewportY{ 0.0f }, ViewportWidth{ 0.0f }, ViewportHeight{ 0.0f };
        DepthTargetInfo Depth{};        // the depth stencil view bound at the draw (VW.6.y.2)
    };

    // Name of a DXGI format the render passes use (color and depth). return: Null for the others.
    inline auto FormatName(std::uint32_t format) -> const char*
    {
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:         return "R8G8B8A8_TYPELESS";
        case DXGI_FORMAT_R8G8B8A8_UNORM:            return "R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:       return "R8G8B8A8_UNORM_SRGB";
        case DXGI_FORMAT_B8G8R8A8_UNORM:            return "B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:       return "B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_R10G10B10A2_UNORM:         return "R10G10B10A2_UNORM";
        case DXGI_FORMAT_R16G16B16A16_FLOAT:        return "R16G16B16A16_FLOAT";
        case DXGI_FORMAT_R11G11B10_FLOAT:           return "R11G11B10_FLOAT";
        case DXGI_FORMAT_R32_TYPELESS:              return "R32_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT:                 return "D32_FLOAT";
        case DXGI_FORMAT_R32_FLOAT:                 return "R32_FLOAT";
        case DXGI_FORMAT_R24G8_TYPELESS:            return "R24G8_TYPELESS";
        case DXGI_FORMAT_D24_UNORM_S8_UINT:         return "D24_UNORM_S8_UINT";
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:     return "R24_UNORM_X8_TYPELESS";
        case DXGI_FORMAT_R32G8X24_TYPELESS:         return "R32G8X24_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:      return "D32_FLOAT_S8X24_UINT";
        case DXGI_FORMAT_R16_TYPELESS:              return "R16_TYPELESS";
        case DXGI_FORMAT_D16_UNORM:                 return "D16_UNORM";
        default:                                    return nullptr;
        }
    }

    // The callers of the first person model pass that the probe tells apart (VW.6.y.2). Each is a function of the game that calls it.
    enum class FirstPersonCaller : std::uint8_t
    {
        Pass1,          // 0x26DA08 (called by 0x26C6DC with 0xB and by 0x26D740)
        Func26E2A0,     // 0x26E2A0
        ViewPhase,      // 0x26EA78 (phase 0 or 1 of the view)
        Func277D00,     // 0x277D00 (no known direct caller)
        Other           // any other return address, the raw RVA is kept
    };

    inline constexpr std::size_t k_FirstPersonCallerCount{ 5 };

    // A function of the game by its first byte and its size, as the decompiled code gives them.
    struct FirstPersonCallerRange
    {
        FirstPersonCaller Caller{ FirstPersonCaller::Other };
        std::uint32_t Rva{ 0 };
        std::uint32_t Size{ 0 };
    };

    inline constexpr std::array<FirstPersonCallerRange, 4> k_FirstPersonCallerRanges
    { {
        { FirstPersonCaller::Pass1,      0x26DA08, 498 },
        { FirstPersonCaller::Func26E2A0, 0x26E2A0, 605 },
        { FirstPersonCaller::ViewPhase,  0x26EA78, 788 },
        { FirstPersonCaller::Func277D00, 0x277D00, 543 }
    } };

    // The color target in slot 0: its texture, its view and whether it is the back buffer.
    struct ColorTargetInfo
    {
        bool HasTarget{ false };
        bool IsBackBuffer{ false };     // the resource is the one of the back buffer view of the store
        bool IsSameDevice{ false };     // the target belongs to the device of the store
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        std::uint32_t ResourceFormat{ 0 };  // DXGI_FORMAT of the texture
        std::uint32_t ViewFormat{ 0 };      // DXGI_FORMAT of the view (0 = same as the texture)
    };

    struct ViewportInfo
    {
        bool HasViewport{ false };
        float X{ 0.0f }, Y{ 0.0f }, Width{ 0.0f }, Height{ 0.0f };
        float MinDepth{ 0.0f }, MaxDepth{ 0.0f };
    };

    // The depth stencil state that was bound. HasState false is the default state of D3D11 (a null state).
    struct DepthStencilStateInfo
    {
        bool HasState{ false };
        bool IsDepthEnabled{ false };
        bool IsStencilEnabled{ false };
        std::uint32_t DepthWriteMask{ 0 };      // D3D11_DEPTH_WRITE_MASK
        std::uint32_t DepthFunc{ 0 };           // D3D11_COMPARISON_FUNC
        std::uint32_t StencilReadMask{ 0 };
        std::uint32_t StencilWriteMask{ 0 };
        std::uint32_t StencilRef{ 0 };          // read even when the state is null
        std::uint32_t FrontFunc{ 0 };           // D3D11_COMPARISON_FUNC
        std::uint32_t FrontPassOp{ 0 };         // D3D11_STENCIL_OP
        std::uint32_t FrontFailOp{ 0 };
        std::uint32_t FrontDepthFailOp{ 0 };
    };

    // The rasterizer state that was bound. HasState false is the default state (a null state).
    struct RasterizerStateInfo
    {
        bool HasState{ false };
        bool IsDepthClipEnabled{ false };
        std::int32_t DepthBias{ 0 };
        float SlopeScaledDepthBias{ 0.0f };
        std::uint32_t CullMode{ 0 };            // D3D11_CULL_MODE
    };

    // What the first person model detour measured at a call for user 0 (VW.6.y.2). Only reads, never drawn from.
    struct FirstPersonSample
    {
        bool HasSample{ false };
        bool IsSameThread{ false };     // it ran on the thread of Present. When false the next fields are not read
        FirstPersonCaller Caller{ FirstPersonCaller::Other };
        std::uint32_t CallerRva{ 0 };   // return address minus the base of haloreach.dll
        std::uint32_t Pass{ 0 };        // the uint8 argument of the model pass
        std::uint32_t CallsPerFrame{ 0 };   // calls of this caller between the last two Present
        std::uint32_t RenderTargetCount{ 0 };   // non null render targets bound
        ColorTargetInfo Color{};        // render target 0
        DepthTargetInfo Depth{};
        ViewportInfo Viewport{};        // viewport 0
        DepthStencilStateInfo DepthState{};
        RasterizerStateInfo Rasterizer{};
    };

    // Where the stencil of the game is reduced in the probe (VW.6.y.2 A2), in texels of the depth stencil texture. Weapon is the lower right
    // quadrant, Center a box of 10% of the width and height in the middle, Top the upper quarter.
    enum class StencilRegion : std::uint8_t
    {
        All,
        Weapon,
        Center,
        Top
    };

    inline constexpr std::size_t k_StencilRegionCount{ 4 };
    inline constexpr std::size_t k_StencilValueCount{ 256 };
    inline constexpr std::size_t k_StencilBitCount{ 8 };

    // What happened reading the depth stencil texture of the game.
    enum class StencilReadStatus : std::uint8_t
    {
        NotRead,
        Read,
        NotTexture,         // the resource is not a 2D texture
        UnsupportedFormat,  // not R32G8X24 (typeless or D32_FLOAT_S8X24_UINT)
        CreateFailed,       // the staging texture could not be created (Result is the HRESULT)
        MapFailed,          // Map(READ) of the staging copy failed (Result is the HRESULT)
        BadLayout           // no data or a row pitch smaller than a row
    };

    // The stencil and depth of one region: texel count, 256 stencil counters, how many texels have each bit set, depth range.
    struct StencilRegionStats
    {
        std::uint32_t Texels{ 0 };
        std::array<std::uint32_t, k_StencilValueCount> Histogram{};
        std::array<std::uint32_t, k_StencilBitCount> BitCounts{};
        float MinDepth{ 0.0f };
        float MaxDepth{ 0.0f };
        std::uint32_t ZeroDepthTexels{ 0 };     // reversed Z: 0 is far or sky
    };

    // One read of the stencil of the first person depth stencil resource at the HUD draw (VW.6.y.2 A2). Fixed size, no allocation.
    struct StencilProbeInfo
    {
        bool IsAttempted{ false };      // the read was tried at a HUD draw of this probe
        StencilReadStatus Status{ StencilReadStatus::NotRead };
        std::int32_t Result{ 0 };       // HRESULT of the step that failed
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        std::uint32_t ResourceFormat{ 0 };  // DXGI_FORMAT of the texture
        std::array<StencilRegionStats, k_StencilRegionCount> Regions{};
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

    // Handler that draws under the HUD of the game. return: True if it drew, so the Present handlers skip that work in the frame.
    using BeforeHudHandler = std::function<bool(const FrameContext&)>;
    using ShutdownCallback = std::function<void()>;

    // Longest wait of a render hook for the frame in flight when it is removed.
    inline constexpr std::chrono::milliseconds k_IdleTimeout{ 1000 };
}