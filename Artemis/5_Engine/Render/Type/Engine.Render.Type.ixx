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

    // The opaque color pass of the view 0x26E500: void(void* view).
    using OpaqueColorPassFunction = auto(__fastcall*)(void* view) -> void;

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
        NoSceneCall,        // (scene) the opaque color pass was not called in the frame, or the option is off
        Failed              // a handler threw: the frame counts as handled, so Present does not draw it again
    };

    // Where the overlay was drawn in the last completed Present frame (VW.6.z).
    enum class DrawPlace : std::uint8_t
    {
        Present,    // at Present, after the frame of the game (no earlier point drew it)
        Hud,        // before the HUD of the game, after the post
        Scene       // inside the frame, at the exit of the opaque color pass 0x26E500
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

    // The copies of the overlay mask: the depth around the call with pass 0xB and the depth around the call with pass 0. Each one is a pair of
    // textures taken before and after the call.
    enum class MaskPairKind : std::uint8_t
    {
        DepthB,
        DepthZero
    };

    inline constexpr std::size_t k_MaskPairCount{ 2 };

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

    // The constant buffer b1 of the pixel shader, 32 bytes as the shader declares it.
    struct MaskConstants
    {
        std::uint32_t DepthBWidth{ 1 };
        std::uint32_t DepthBHeight{ 1 };
        std::uint32_t DepthZeroWidth{ 1 };
        std::uint32_t DepthZeroHeight{ 1 };
        std::uint32_t TargetWidth{ 1 };
        std::uint32_t TargetHeight{ 1 };
        std::uint32_t ValidFlags{ 0 };      // bit i: the pair of MaskPairKind i is used
        float ColorScale{ 1.0f };           // the pixel shader multiplies the RGB it outputs by it (VW.6.z.2)
    };
    static_assert(sizeof(MaskConstants) == 32);

    // return: Whether the pair can be sampled: valid, both views, a size and a target size, so the shader never divides by zero.
    constexpr auto IsMaskPairUsable(const MaskPairView& pair, std::uint32_t targetWidth, std::uint32_t targetHeight) -> bool
    {
        return pair.IsValid && pair.Pre != nullptr && pair.Post != nullptr && pair.Width != 0 && pair.Height != 0 && targetWidth != 0 &&
            targetHeight != 0;
    }

    // Makes the constants of the pixel shader of a mask.
    // param colorScale: The factor of the RGB output (the scene color scale, 1 outside the scene). It does not depend on the mask.
    // return: The flags hold the usable pairs only, and the sizes of a pair that is not usable stay 1.
    inline auto MakeMaskConstants(const MaskContext& mask, std::uint32_t targetWidth, std::uint32_t targetHeight, float colorScale = 1.0f)
        -> MaskConstants
    {
        MaskConstants constants{};
        constants.ColorScale = colorScale;

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
            }
        }

        if (constants.ValidFlags != 0)
        {
            constants.TargetWidth = targetWidth;
            constants.TargetHeight = targetHeight;
        }

        return constants;
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

        // Factor the 3D passes of the overlay multiply the RGB they output by (VW.6.z.2). 1 everywhere but at the scene point, where the HDR target is not
        // in the 0..1 range of the back buffer.
        float ColorScale{ 1.0f };
    };

    using FrameHandler = std::function<void(const FrameContext&)>;

    // Handler that draws under the HUD of the game. return: True if it drew, so the Present handlers skip that work in the frame.
    using BeforeHudHandler = std::function<bool(const FrameContext&)>;
    using ShutdownCallback = std::function<void()>;

    // Longest wait of a render hook for the frame in flight when it is removed.
    inline constexpr std::chrono::milliseconds k_IdleTimeout{ 1000 };
}