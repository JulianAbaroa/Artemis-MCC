module;

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

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

    // The pass 0x26DA08 between the entry and the exit of the first person draw: void(void* view, int32 pass).
    using FirstPersonDepthPassFunction = auto(__fastcall*)(void* view, std::int32_t pass) -> void;

    // The forward pass of the view 0x26EA78: void(void* view, uint8 phase). The phase is a char in DL.
    using FirstPersonForwardFunction = auto(__fastcall*)(void* view, std::uint8_t phase) -> void;

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
        Multisampled,       // (scene) the bound target has more than one sample
        NoSceneCall,        // (scene) the forward pass was not called with phase 0 in the frame, or the option is off
        Failed              // a handler threw: the frame counts as handled, so Present does not draw it again
    };

    // Where the overlay was drawn in the last completed Present frame (VW.6.z).
    enum class DrawPlace : std::uint8_t
    {
        Present,    // at Present, after the frame of the game (no earlier point drew it)
        Hud,        // before the HUD of the game, after the post
        Scene       // inside the frame, at the entry of the forward pass 0x26EA78 with phase 0
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
        std::uint32_t SampleCount{ 1 }; // samples of the target texture
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
        std::uint32_t SampleCount{ 1 };     // samples of the texture
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

    // How many calls of the first person pass 0x26DA08 the probe copies the depth of, before and after (VW.6.y.2 A4, A5). Two textures each.
    inline constexpr std::size_t k_DepthPairCount{ 6 };

    // How many calls of the forward pass 0x26EA78 the probe copies the color of, before and after (VW.6.y.2 D). Two textures each.
    inline constexpr std::size_t k_ColorPairCount{ 4 };

    // What the detour did with a call while the copy pairs (depth or color) were ready.
    enum class PairState : std::uint8_t
    {
        Free,
        Mismatch,   // the bound resource is not a texture of the description of the copies, nothing was copied
        Copied,     // the content before the call was copied, and the content after it when the call returned
        NoTarget    // nothing was bound to copy from (color pairs: no render target 0)
    };

    // One call of the pass seen while the depth pairs were ready, in call order.
    struct DepthPairNote
    {
        PairState State{ PairState::Free };
        FirstPersonCaller Caller{ FirstPersonCaller::Other };   // Pass1 (0x26DA08)
        std::int32_t Pass{ 0 };                                 // the pass argument of 0x26DA08
        bool IsCaptured{ false };   // no depth stencil was bound at the entry, the resource captured from the model draw was copied instead
    };

    // How the creation of the copy textures of the depth pairs or the color pairs went (once per probe, at Present).
    struct CopyPairSetup
    {
        bool IsAttempted{ false };
        StencilReadStatus Status{ StencilReadStatus::NotRead };     // Read when created
        std::int32_t Result{ 0 };       // HRESULT of the step that failed
    };

    // How a texel of a copy is compared (VW.6.y.2 D): its size, how many of its first bytes are compared, and whether those bytes are a float
    // depth whose range is reported.
    struct TexelLayout
    {
        std::uint32_t TexelBytes{ 0 };
        std::uint32_t CompareBytes{ 0 };
        bool HasDepthRange{ false };
    };

    // R32G8X24: the float depth in bytes 0..3, the stencil and 3 unused bytes.
    inline constexpr TexelLayout k_DepthTexelLayout{ 8, 4, true };

    // A color of 4 bytes per texel (R11G11B10_FLOAT and the 8 bit formats): all the bytes are compared.
    inline constexpr TexelLayout k_ColorTexelLayout{ 4, 4, false };

    // A byte per texel of an image, 1 where the texel changed, row after row without padding. Empty when none was made.
    struct ChangedMap
    {
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        std::vector<std::uint8_t> Texels{};
    };

    // Texels that changed between two copies: count, box (inclusive), depth range of the later copy (0 for a color), how many are in the
    // weapon region and how many in the right half of the screen (x from width / 2). With Changed 0 the box and the range are 0.
    struct TexelDiff
    {
        std::uint32_t Changed{ 0 };
        std::uint32_t X0{ 0 };
        std::uint32_t Y0{ 0 };
        std::uint32_t X1{ 0 };
        std::uint32_t Y1{ 0 };
        float MinDepth{ 0.0f };
        float MaxDepth{ 0.0f };
        std::uint32_t Weapon{ 0 };
        std::uint32_t Right{ 0 };
    };

    // The reduction of one pair: what the call changed and, when the previous pair was read too, what changed between the two calls.
    struct DepthPairReport
    {
        DepthPairNote Note{};
        StencilReadStatus Status{ StencilReadStatus::NotRead };     // Read when Changed is valid
        std::int32_t Result{ 0 };
        TexelDiff Changed{};
        bool HasBetween{ false };
        TexelDiff Between{};            // previous pair after the call against this pair before it
    };

    struct DepthProbeInfo
    {
        std::uint32_t Count{ 0 };
        std::array<DepthPairReport, k_DepthPairCount> Pairs{};
    };

    // One call of the forward pass 0x26EA78 seen while the color pairs were ready, in call order.
    struct ColorPairNote
    {
        PairState State{ PairState::Free };
        std::uint8_t Phase{ 0 };    // the phase argument of 0x26EA78
    };

    // The reduction of one color pair: what the call changed, and how many of those texels are not in the mask of the depth pair 0xB (nothing
    // when there is no such mask or its size differs).
    struct ColorPairReport
    {
        ColorPairNote Note{};
        StencilReadStatus Status{ StencilReadStatus::NotRead };     // Read when Changed is valid
        std::int32_t Result{ 0 };
        TexelDiff Changed{};
        std::optional<std::uint32_t> Outside{};
    };

    struct ColorProbeInfo
    {
        std::uint32_t Count{ 0 };
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        std::uint32_t Format{ 0 };      // DXGI_FORMAT of the copies
        std::array<ColorPairReport, k_ColorPairCount> Pairs{};
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

    // The passes of 0x26DA08 that draw the first person arms and weapon in the depth of the view (VW.6.y.2 A5, A6, E). The game draws them in
    // the first call with pass 0xB, or, when a mesh of the previous frame sets a flag of its shader (a Ghost, a Revenant, the fuel rod), in the
    // first call with pass 0 after the scene. The overlay mask is made around both.
    inline constexpr std::int32_t k_FirstPersonMaskPass{ 0xB };
    inline constexpr std::int32_t k_FirstPersonZeroPass{ 0 };

    // The first phase of the forward pass 0x26EA78 is the depth mask. The phases after it draw the emissive parts that write no depth, such as
    // the blade of the sword (VW.6.y.2 A6).
    inline constexpr std::uint8_t k_ForwardDepthPhase{ 0 };

    // The copies of the overlay mask: the depth around the call with pass 0xB, the depth around the call with pass 0 and the color of the
    // forward pass 0x26EA78. Each one is a pair of textures taken before and after the call.
    enum class MaskPairKind : std::uint8_t
    {
        DepthB,
        DepthZero,
        Color
    };

    inline constexpr std::size_t k_MaskPairCount{ 3 };

    // Fixed slots of the overlay mask in the pixel stage of the map shaders (VW.6.y.2 B, E): the before and after copy of each pair
    // (shader resources, consecutive from slot 0 in the order of MaskPairKind) and the constant buffer with the sizes and flags.
    inline constexpr UINT k_MaskSlotCount{ static_cast<UINT>(k_MaskPairCount) * 2 };
    inline constexpr UINT k_MaskBufferSlot{ 1 };

    // return: The shader resource slot of the copy before (PreSlot) or after (PostSlot) the call of the pair.
    constexpr auto MaskPreSlot(MaskPairKind kind) -> UINT
    {
        return static_cast<UINT>(kind) * 2;
    }

    constexpr auto MaskPostSlot(MaskPairKind kind) -> UINT
    {
        return static_cast<UINT>(kind) * 2 + 1;
    }

    // What to do with the mask textures of a pair at Present (RenderService::PlanMask).
    enum class MaskPlan : std::uint8_t
    {
        Keep,       // nothing to do
        Create,     // make the textures (again) from the wanted description
        Release     // the mask is not requested any more
    };

    // Where the pass 0 copy takes its resource from (VW.6.y.2 E).
    enum class MaskSource : std::uint8_t
    {
        Bound,      // the depth stencil bound at the entry of the call
        Saved,      // none bound: the one of the call with pass 0xB of the same frame (the same resource in the whole view)
        None        // neither: the pair is not copied in this call
    };

    // return: The resource the pass 0 copy uses. The bound one wins, as the game writes there when it binds one.
    constexpr auto ChooseZeroMaskSource(bool isBound, bool isSaved) -> MaskSource
    {
        if (isBound) return MaskSource::Bound;
        return isSaved ? MaskSource::Saved : MaskSource::None;
    }

    // return: Whether a pair is wanted. The color pair is the one of the forward pass phase 1, drawn after the scene point, so with the overlay
    // drawn in the scene (VW.6.z) it is not wanted.
    constexpr auto IsMaskPairWanted(bool isMaskRequested, bool isSceneRequested, MaskPairKind kind) -> bool
    {
        return isMaskRequested && !(isSceneRequested && kind == MaskPairKind::Color);
    }

    // One pair of copies of the mask for the overlay. Without IsValid the pair is not used.
    struct MaskPairView
    {
        bool IsValid{ false };                                  // the copies were taken in the frame this context is for
        ID3D11ShaderResourceView* Pre{ nullptr };               // the texture at the entry of the call
        ID3D11ShaderResourceView* Post{ nullptr };              // the texture at the exit of the call
        std::uint32_t Width{ 0 };                               // size of the copies, which may differ from the target
        std::uint32_t Height{ 0 };
    };

    // The textures of one pair of the overlay mask (VW.6.y.2 B, E): the copy before and after the call, their shader resource views, the
    // description of the resource the pair is made from, the description wanted and whether a creation was attempted for it.
    // note: Touched on the thread of Present only (the detours copy there too), no lock.
    struct MaskTextures
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> Pre{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> Post{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> PreView{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> PostView{};
        Microsoft::WRL::ComPtr<ID3D11Resource> Source{};        // the resource of the call in flight, so the exit copies the same one
        D3D11_TEXTURE2D_DESC Desc{};                            // the description of the last attempt
        std::optional<D3D11_TEXTURE2D_DESC> Wanted{};           // the description noted when the copies could not be taken
        bool IsAttempted{ false };
    };

    // The first person arms as pairs of copies taken around the passes that draw them (VW.6.y.2 B, E). The overlay skips the texels where any
    // valid pair differs between its two copies.
    struct MaskContext
    {
        std::array<MaskPairView, k_MaskPairCount> Pairs{};
    };

    // The constant buffer b1 of the pixel shader, 48 bytes as the shader declares it.
    struct MaskConstants
    {
        std::uint32_t DepthBWidth{ 1 };
        std::uint32_t DepthBHeight{ 1 };
        std::uint32_t DepthZeroWidth{ 1 };
        std::uint32_t DepthZeroHeight{ 1 };
        std::uint32_t ColorWidth{ 1 };
        std::uint32_t ColorHeight{ 1 };
        std::uint32_t TargetWidth{ 1 };
        std::uint32_t TargetHeight{ 1 };
        std::uint32_t ValidFlags{ 0 };      // bit i: the pair of MaskPairKind i is used
        std::uint32_t MaskShown{ 0 };
        std::uint32_t Padding[2]{};
    };
    static_assert(sizeof(MaskConstants) == 48);

    // return: Whether the pair can be sampled: valid, both views, a size and a target size, so the shader never divides by zero.
    constexpr auto IsMaskPairUsable(const MaskPairView& pair, std::uint32_t targetWidth, std::uint32_t targetHeight) -> bool
    {
        return pair.IsValid && pair.Pre != nullptr && pair.Post != nullptr && pair.Width != 0 && pair.Height != 0 && targetWidth != 0 &&
            targetHeight != 0;
    }

    // Makes the constants of the pixel shader of a mask.
    // param isMaskShown: Debug. The pixels of the mask are painted instead of skipped. Ignored when no pair is usable.
    // return: The flags hold the usable pairs only, and the sizes of a pair that is not usable stay 1.
    inline auto MakeMaskConstants(const MaskContext& mask, bool isMaskShown, std::uint32_t targetWidth, std::uint32_t targetHeight)
        -> MaskConstants
    {
        MaskConstants constants{};

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i)
        {
            const MaskPairView& pair{ mask.Pairs[i] };
            if (!IsMaskPairUsable(pair, targetWidth, targetHeight)) continue;

            constants.ValidFlags |= 1u << i;

            switch (static_cast<MaskPairKind>(i))
            {
            case MaskPairKind::DepthB:
                constants.DepthBWidth = pair.Width;
                constants.DepthBHeight = pair.Height;
                break;

            case MaskPairKind::DepthZero:
                constants.DepthZeroWidth = pair.Width;
                constants.DepthZeroHeight = pair.Height;
                break;

            case MaskPairKind::Color:
                constants.ColorWidth = pair.Width;
                constants.ColorHeight = pair.Height;
                break;
            }
        }

        if (constants.ValidFlags != 0)
        {
            constants.TargetWidth = targetWidth;
            constants.TargetHeight = targetHeight;
            constants.MaskShown = isMaskShown ? 1u : 0u;
        }

        return constants;
    }

    // return: The format a shader resource view of a color texture of the mask uses, nothing when the format is not one the mask compares.
    // A typeless 8 bit format is viewed as UNORM.
    inline auto ColorMaskViewFormat(DXGI_FORMAT format) -> std::optional<DXGI_FORMAT>
    {
        switch (format)
        {
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return format;

        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        default:
            return std::nullopt;
        }
    }

    // Frames of a telemetry window of the overlay mask, counted only while the mask is requested (VW.6.y.2 C, E).
    inline constexpr std::uint32_t k_MaskWindowFrames{ 120 };

    // Counters of the window in progress.
    struct MaskWindow
    {
        std::uint32_t Frames{ 0 };                                          // frames with the mask requested
        std::array<std::uint32_t, k_MaskPairCount> NoCopyFrames{};          // of those, frames in which each pair took no copy
        std::uint32_t MaxPassCalls{ 0 };                                    // most 0xB calls seen in one frame
    };

    // A closed window, published for the telemetry.
    struct MaskWindowReport
    {
        std::uint32_t Frames{ 0 };
        std::array<std::uint32_t, k_MaskPairCount> NoCopyFrames{};
        std::uint32_t MaxPassCalls{ 0 };
    };

    // Adds one frame with the mask requested to the window.
    // param isCopied: Whether each pair took its copies in the frame.
    // param passCalls: Calls of 0xB that asked for the mask in the frame, including those that did not copy.
    // return: The window when it reached k_MaskWindowFrames, which is then reset. Empty otherwise.
    inline auto AdvanceMaskWindow(MaskWindow& window, const std::array<bool, k_MaskPairCount>& isCopied, std::uint32_t passCalls)
        -> std::optional<MaskWindowReport>
    {
        ++window.Frames;

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i)
        {
            if (!isCopied[i]) ++window.NoCopyFrames[i];
        }

        window.MaxPassCalls = (std::max)(window.MaxPassCalls, passCalls);

        if (window.Frames < k_MaskWindowFrames) return std::nullopt;

        const MaskWindowReport report{ window.Frames, window.NoCopyFrames, window.MaxPassCalls };
        window = MaskWindow{};
        return report;
    }

    // Adds the texels of a changed map to another, for the union of the depth the first person passes wrote (VW.6.y.2 E).
    // return: False when the sizes differ and nothing was added. An empty target takes the size of the source.
    inline auto UnionChangedMap(ChangedMap& target, const ChangedMap& source) -> bool
    {
        if (source.Texels.empty()) return true;

        if (target.Texels.empty())
        {
            target = source;
            return true;
        }

        if (target.Width != source.Width || target.Height != source.Height || target.Texels.size() != source.Texels.size()) return false;

        for (std::size_t i{ 0 }; i < target.Texels.size(); ++i) target.Texels[i] |= source.Texels[i];
        return true;
    }

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

        MaskContext Mask{};     // the first person mask of this frame, used by the overlay passes
    };

    using FrameHandler = std::function<void(const FrameContext&)>;

    // Handler that draws under the HUD of the game. return: True if it drew, so the Present handlers skip that work in the frame.
    using BeforeHudHandler = std::function<bool(const FrameContext&)>;
    using ShutdownCallback = std::function<void()>;

    // Longest wait of a render hook for the frame in flight when it is removed.
    inline constexpr std::chrono::milliseconds k_IdleTimeout{ 1000 };
}