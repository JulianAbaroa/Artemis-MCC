module;

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

module Engine.Render.System;
import :RenderService;
import :GpuStateGuard;
import std;

namespace
{
    using Microsoft::WRL::ComPtr;

    using Engine::Render::State::CopyPairs;

    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::ChangedMap;
    using Engine::Render::Type::ColorPairNote;
    using Engine::Render::Type::ColorPairReport;
    using Engine::Render::Type::ColorProbeInfo;
    using Engine::Render::Type::ColorTargetInfo;
    using Engine::Render::Type::CopyPairSetup;
    using Engine::Render::Type::DepthPairNote;
    using Engine::Render::Type::DepthPairReport;
    using Engine::Render::Type::DepthProbeInfo;
    using Engine::Render::Type::DepthStencilStateInfo;
    using Engine::Render::Type::DepthTargetInfo;
    using Engine::Render::Type::DrawPlace;
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::FirstPersonSample;
    using Engine::Render::Type::FormatName;
    using Engine::Render::Type::HudHookInfo;
    using Engine::Render::Type::MaskContext;
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::MaskPairView;
    using Engine::Render::Type::MaskPlan;
    using Engine::Render::Type::MaskSource;
    using Engine::Render::Type::PairState;
    using Engine::Render::Type::RasterizerStateInfo;
    using Engine::Render::Type::StencilProbeInfo;
    using Engine::Render::Type::StencilReadStatus;
    using Engine::Render::Type::StencilRegion;
    using Engine::Render::Type::StencilRegionStats;
    using Engine::Render::Type::TexelDiff;
    using Engine::Render::Type::TexelLayout;
    using Engine::Render::Type::ViewportInfo;

    using Engine::Render::Type::ChooseZeroMaskSource;
    using Engine::Render::Type::ColorMaskViewFormat;
    using Engine::Render::Type::IsMaskPairWanted;
    using Engine::Render::Type::UnionChangedMap;

    using Engine::Render::Type::k_ColorPairCount;
    using Engine::Render::Type::k_ColorTexelLayout;
    using Engine::Render::Type::k_DepthPairCount;
    using Engine::Render::Type::k_DepthTexelLayout;
    using Engine::Render::Type::k_FirstPersonCallerCount;
    using Engine::Render::Type::k_FirstPersonCallerRanges;
    using Engine::Render::Type::k_FirstPersonMaskPass;
    using Engine::Render::Type::k_FirstPersonZeroPass;
    using Engine::Render::Type::k_ForwardDepthPhase;
    using Engine::Render::Type::k_MaskPairCount;
    using Engine::Render::Type::k_StencilBitCount;
    using Engine::Render::Type::k_StencilRegionCount;
    using Engine::Render::Type::k_StencilValueCount;

    using std::chrono::steady_clock;
    using std::chrono::microseconds;

    // return: Why a draw of the overlay cannot use the target bound in slot 0, or nothing. The HUD draw needs the back buffer, the scene point the
    // HDR target of the game (one sample, the size of the back buffer).
    auto FindTargetBlocker(const HudHookInfo& sample, bool isReady, std::uint32_t width, std::uint32_t height, bool isInScene)
        -> std::optional<BeforeHudStatus>
    {
        if (!sample.IsSameThread) return BeforeHudStatus::OtherThread;
        if (!isReady) return BeforeHudStatus::NotReady;
        if (!sample.HasTarget || width == 0 || height == 0) return BeforeHudStatus::TargetMismatch;
        if (!sample.IsSameDevice) return BeforeHudStatus::OtherDevice;
        if (!isInScene && !sample.IsBackBuffer) return BeforeHudStatus::NotBackBuffer;
        if (sample.Width != width || sample.Height != height) return BeforeHudStatus::TargetMismatch;
        if (isInScene && sample.SampleCount != 1) return BeforeHudStatus::Multisampled;

        const bool isWholeTarget{ sample.HasViewport && sample.ViewportX == 0.0f && sample.ViewportY == 0.0f &&
            sample.ViewportWidth == static_cast<float>(width) && sample.ViewportHeight == static_cast<float>(height) };
        if (!isWholeTarget) return BeforeHudStatus::SplitViewport;

        return std::nullopt;
    }

    // How the log names the first person pass of a depth pair: the function 0x26DA08 and its pass argument.
    auto DepthCallerName(const DepthPairNote& note) -> std::string
    {
        return std::format("26DA08 pass {}", note.Pass);
    }

    // return: The resource of the depth stencil view bound now, null when none.
    auto BoundDepthResource(ID3D11DeviceContext* context) -> ComPtr<ID3D11Resource>
    {
        ComPtr<ID3D11DepthStencilView> view{};
        context->OMGetRenderTargets(0, nullptr, view.GetAddressOf());

        ComPtr<ID3D11Resource> resource{};
        if (view != nullptr) view->GetResource(resource.GetAddressOf());
        return resource;
    }

    // Bytes of a texel of R32G8X24: float depth, stencil byte, 3 unused.
    constexpr std::uint32_t k_StencilTexelBytes{ 8 };
    constexpr std::uint32_t k_StencilByteOffset{ 4 };
    constexpr std::size_t k_StencilTopCount{ 8 };

    // return: The resource of the render target view bound in slot 0 now, null when none.
    auto BoundColorResource(ID3D11DeviceContext* context) -> ComPtr<ID3D11Resource>
    {
        ComPtr<ID3D11RenderTargetView> view{};
        context->OMGetRenderTargets(1, view.GetAddressOf(), nullptr);

        ComPtr<ID3D11Resource> resource{};
        if (view != nullptr) view->GetResource(resource.GetAddressOf());
        return resource;
    }

    // Whether a copy made from a can take the content of a texture of description b.
    auto IsSameTextureDesc(const D3D11_TEXTURE2D_DESC& a, const D3D11_TEXTURE2D_DESC& b) -> bool
    {
        return a.Width == b.Width && a.Height == b.Height && a.MipLevels == b.MipLevels && a.ArraySize == b.ArraySize &&
            a.Format == b.Format && a.SampleDesc.Count == b.SampleDesc.Count && a.SampleDesc.Quality == b.SampleDesc.Quality;
    }

    // return: How the log names a pair of the overlay mask.
    auto MaskPairName(MaskPairKind kind) -> const char*
    {
        switch (kind)
        {
        case MaskPairKind::DepthB: return "depth 0xB";
        case MaskPairKind::DepthZero: return "depth 0";
        case MaskPairKind::Color: return "color";
        }

        return "?";
    }

    // The textures of a pair of the overlay mask as the creation left them.
    struct MaskCreation
    {
        std::array<ComPtr<ID3D11Texture2D>, 2> Textures{};
        std::array<ComPtr<ID3D11ShaderResourceView>, 2> Views{};
        HRESULT Result{ S_OK };
    };

    // Makes the before and after textures of a pair, DEFAULT with only the shader resource bind, and a view of each with the format.
    auto CreateMaskTextures(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& wanted, DXGI_FORMAT viewFormat) -> MaskCreation
    {
        D3D11_TEXTURE2D_DESC copyDesc{ wanted };
        copyDesc.Usage = D3D11_USAGE_DEFAULT;
        copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        copyDesc.CPUAccessFlags = 0;
        copyDesc.MiscFlags = 0;

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
        viewDesc.Format = viewFormat;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.MipLevels = 1;

        MaskCreation creation{ .Result = device != nullptr ? S_OK : E_POINTER };

        for (std::size_t i{ 0 }; i < creation.Textures.size() && SUCCEEDED(creation.Result); ++i)
        {
            creation.Result = device->CreateTexture2D(&copyDesc, nullptr, creation.Textures[i].GetAddressOf());

            if (SUCCEEDED(creation.Result))
            {
                creation.Result = device->CreateShaderResourceView(creation.Textures[i].Get(), &viewDesc, creation.Views[i].GetAddressOf());
            }
        }

        return creation;
    }

    // return: The bytes of a texel of a color format the color pairs compare, 0 for any other.
    auto ColorTexelBytes(DXGI_FORMAT format) -> std::uint32_t
    {
        switch (format)
        {
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return k_ColorTexelLayout.TexelBytes;

        default:
            return 0;
        }
    }

    // Notes a call of a pass and, when the bound resource is a texture of the description of the copies, copies it to the before texture of
    // the pair. The copy is the same for the depth pairs and the color pairs (VW.6.y.2 A5, D).
    // param note: State is set here.
    // return: The pair to give to EndPairCopy, nothing when no copy was made (mismatch, or the pairs are taken).
    template <typename Note, std::size_t Count>
    auto BeginPairCopy(ID3D11DeviceContext* context, CopyPairs<Note, Count>& pairs, ID3D11Resource* resource, Note note)
        -> std::optional<std::size_t>
    {
        ComPtr<ID3D11Texture2D> texture{};
        if (resource != nullptr) resource->QueryInterface(IID_PPV_ARGS(texture.GetAddressOf()));

        D3D11_TEXTURE2D_DESC desc{};
        if (texture != nullptr) texture->GetDesc(&desc);

        if (texture == nullptr || !IsSameTextureDesc(desc, pairs.GetDesc()))
        {
            note.State = PairState::Mismatch;
            pairs.NoteCall(note, nullptr);
            return std::nullopt;
        }

        note.State = PairState::Copied;
        const std::optional<std::size_t> pair{ pairs.NoteCall(note, resource) };
        if (!pair) return std::nullopt;

        context->CopyResource(pairs.GetCopy(*pair * 2), resource);
        return pair;
    }

    // Copies the resource of the pair to its after texture.
    template <typename Note, std::size_t Count>
    auto EndPairCopy(ID3D11DeviceContext* context, const CopyPairs<Note, Count>& pairs, std::size_t pair) -> void
    {
        ID3D11Resource* source{ pairs.GetSource(pair) };
        ID3D11Texture2D* after{ pairs.GetCopy(pair * 2 + 1) };
        if (context == nullptr || source == nullptr || after == nullptr) return;

        context->CopyResource(after, source);
    }

    // Makes the 2 * Count DEFAULT copy textures and the STAGING one of the description and keeps them in the pairs.
    // return: Status Read, or CreateFailed with the HRESULT.
    template <typename Note, std::size_t Count>
    auto CreateCopyPairs(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc, CopyPairs<Note, Count>& pairs) -> CopyPairSetup
    {
        D3D11_TEXTURE2D_DESC copyDesc{ desc };
        copyDesc.Usage = D3D11_USAGE_DEFAULT;
        copyDesc.BindFlags = 0;
        copyDesc.CPUAccessFlags = 0;
        copyDesc.MiscFlags = 0;

        D3D11_TEXTURE2D_DESC stagingDesc{ copyDesc };
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        std::array<ComPtr<ID3D11Texture2D>, Count * 2> copies{};
        ComPtr<ID3D11Texture2D> staging{};
        HRESULT result{ device != nullptr ? S_OK : E_POINTER };

        for (std::size_t i{ 0 }; i < copies.size() && SUCCEEDED(result); ++i)
        {
            result = device->CreateTexture2D(&copyDesc, nullptr, copies[i].GetAddressOf());
        }

        if (SUCCEEDED(result)) result = device->CreateTexture2D(&stagingDesc, nullptr, staging.GetAddressOf());

        if (FAILED(result) || staging == nullptr)
        {
            return CopyPairSetup{ .IsAttempted = true, .Status = StencilReadStatus::CreateFailed, .Result = static_cast<std::int32_t>(result) };
        }

        pairs.Set(std::move(copies), std::move(staging), desc);
        return CopyPairSetup{ .IsAttempted = true, .Status = StencilReadStatus::Read };
    }

    // Copies a texture to the CPU through the staging, rows packed (width * texelBytes bytes). Blocks until the GPU finishes the copy.
    // return: S_OK, or the HRESULT of the Map (E_INVALIDARG when the pitch is smaller than a row).
    auto CopyTextureToCpu(ID3D11DeviceContext* context, ID3D11Texture2D* staging, ID3D11Texture2D* source, std::uint32_t width,
        std::uint32_t height, std::uint32_t texelBytes, std::vector<std::uint8_t>& out) -> HRESULT
    {
        context->CopyResource(staging, source);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        const HRESULT result{ context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped) };
        if (FAILED(result)) return result;

        const std::size_t rowBytes{ static_cast<std::size_t>(width) * texelBytes };
        HRESULT copyResult{ S_OK };

        if (mapped.RowPitch < rowBytes)
        {
            copyResult = E_INVALIDARG;
        }
        else
        {
            out.resize(rowBytes * height);

            for (std::uint32_t y{ 0 }; y < height; ++y)
            {
                std::memcpy(out.data() + y * rowBytes, static_cast<const std::uint8_t*>(mapped.pData) + static_cast<std::size_t>(y) * mapped.RowPitch,
                    rowBytes);
            }
        }

        context->Unmap(staging, 0);
        return copyResult;
    }

    auto IsInStencilRegion(StencilRegion region, std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height) -> bool
    {
        switch (region)
        {
        case StencilRegion::All:
            return true;
        case StencilRegion::Weapon:
            return x >= width / 2 && y >= height / 2;
        case StencilRegion::Center:
            return x >= width * 45 / 100 && x < width * 55 / 100 && y >= height * 45 / 100 && y < height * 55 / 100;
        case StencilRegion::Top:
            return y < height / 4;
        }

        return false;
    }

    auto StencilRegionName(StencilRegion region) -> const char*
    {
        switch (region)
        {
        case StencilRegion::All:    return "all";
        case StencilRegion::Weapon: return "weapon";
        case StencilRegion::Center: return "center";
        case StencilRegion::Top:    return "top";
        }

        return "?";
    }

    auto AddStencilTexel(StencilRegionStats& stats, float depth, std::uint8_t stencil) -> void
    {
        if (stats.Texels == 0)
        {
            stats.MinDepth = depth;
            stats.MaxDepth = depth;
        }
        else
        {
            stats.MinDepth = (std::min)(stats.MinDepth, depth);
            stats.MaxDepth = (std::max)(stats.MaxDepth, depth);
        }

        ++stats.Texels;
        ++stats.Histogram[stencil];
        if (depth == 0.0f) ++stats.ZeroDepthTexels;

        for (std::size_t bit{ 0 }; bit < k_StencilBitCount; ++bit)
        {
            if (((stencil >> bit) & 1u) != 0) ++stats.BitCounts[bit];
        }
    }

    auto StencilStatusName(StencilReadStatus status) -> const char*
    {
        switch (status)
        {
        case StencilReadStatus::NotRead:            return "not read";
        case StencilReadStatus::Read:               return "read";
        case StencilReadStatus::NotTexture:         return "resource is not a 2D texture";
        case StencilReadStatus::UnsupportedFormat:  return "unsupported format";
        case StencilReadStatus::CreateFailed:       return "staging texture not created";
        case StencilReadStatus::MapFailed:          return "map of the staging copy failed";
        case StencilReadStatus::BadLayout:          return "bad layout";
        }

        return "?";
    }

    // Reads the texture, the view and the identity of a render target. The caller owns the view.
    auto MeasureColorTarget(ID3D11RenderTargetView* target, ID3D11RenderTargetView* backBufferView, ID3D11Device* storeDevice)
        -> ColorTargetInfo
    {
        ColorTargetInfo info{};
        if (target == nullptr) return info;

        D3D11_RENDER_TARGET_VIEW_DESC viewDesc{};
        target->GetDesc(&viewDesc);

        ComPtr<ID3D11Resource> resource{};
        target->GetResource(resource.GetAddressOf());

        ComPtr<ID3D11Texture2D> texture{};
        if (resource == nullptr || FAILED(resource.As(&texture))) return info;

        D3D11_TEXTURE2D_DESC textureDesc{};
        texture->GetDesc(&textureDesc);

        info.HasTarget = true;
        info.Width = textureDesc.Width;
        info.Height = textureDesc.Height;
        info.ResourceFormat = static_cast<std::uint32_t>(textureDesc.Format);
        info.ViewFormat = static_cast<std::uint32_t>(viewDesc.Format);

        ComPtr<ID3D11Resource> backBuffer{};
        if (backBufferView != nullptr) backBufferView->GetResource(backBuffer.GetAddressOf());

        info.SampleCount = textureDesc.SampleDesc.Count;
        info.IsBackBuffer = backBuffer != nullptr && backBuffer.Get() == resource.Get();

        ComPtr<ID3D11Device> targetDevice{};
        target->GetDevice(targetDevice.GetAddressOf());
        info.IsSameDevice = targetDevice != nullptr && targetDevice.Get() == storeDevice;
        return info;
    }

    // Reads the texture and the view of a depth stencil view. The caller owns the view.
    auto MeasureDepthTarget(ID3D11DepthStencilView* view) -> DepthTargetInfo
    {
        DepthTargetInfo info{};
        if (view == nullptr) return info;

        D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc{};
        view->GetDesc(&viewDesc);

        ComPtr<ID3D11Resource> resource{};
        view->GetResource(resource.GetAddressOf());

        info.HasDepth = true;
        info.Resource = reinterpret_cast<std::uintptr_t>(resource.Get());
        info.ViewFormat = static_cast<std::uint32_t>(viewDesc.Format);
        info.ViewFlags = viewDesc.Flags;

        ComPtr<ID3D11Texture2D> texture{};
        if (resource != nullptr && SUCCEEDED(resource.As(&texture)))
        {
            D3D11_TEXTURE2D_DESC textureDesc{};
            texture->GetDesc(&textureDesc);

            info.Width = textureDesc.Width;
            info.Height = textureDesc.Height;
            info.ResourceFormat = static_cast<std::uint32_t>(textureDesc.Format);
        }

        return info;
    }

    auto FormatText(std::uint32_t format) -> std::string
    {
        const char* name{ FormatName(format) };
        return name != nullptr ? std::string{ name } : std::format("fmt {}", format);
    }

    auto ComparisonName(std::uint32_t func) -> std::string
    {
        switch (func)
        {
        case D3D11_COMPARISON_NEVER:            return "NEVER";
        case D3D11_COMPARISON_LESS:             return "LESS";
        case D3D11_COMPARISON_EQUAL:            return "EQUAL";
        case D3D11_COMPARISON_LESS_EQUAL:       return "LESS_EQUAL";
        case D3D11_COMPARISON_GREATER:          return "GREATER";
        case D3D11_COMPARISON_NOT_EQUAL:        return "NOT_EQUAL";
        case D3D11_COMPARISON_GREATER_EQUAL:    return "GREATER_EQUAL";
        case D3D11_COMPARISON_ALWAYS:           return "ALWAYS";
        default:                                return std::format("{}", func);
        }
    }

    auto StencilOpName(std::uint32_t op) -> std::string
    {
        switch (op)
        {
        case D3D11_STENCIL_OP_KEEP:     return "KEEP";
        case D3D11_STENCIL_OP_ZERO:     return "ZERO";
        case D3D11_STENCIL_OP_REPLACE:  return "REPLACE";
        case D3D11_STENCIL_OP_INCR_SAT: return "INCR_SAT";
        case D3D11_STENCIL_OP_DECR_SAT: return "DECR_SAT";
        case D3D11_STENCIL_OP_INVERT:   return "INVERT";
        case D3D11_STENCIL_OP_INCR:     return "INCR";
        case D3D11_STENCIL_OP_DECR:     return "DECR";
        default:                        return std::format("{}", op);
        }
    }

    auto CullName(std::uint32_t mode) -> const char*
    {
        switch (mode)
        {
        case D3D11_CULL_NONE:   return "NONE";
        case D3D11_CULL_FRONT:  return "FRONT";
        case D3D11_CULL_BACK:   return "BACK";
        default:                return "?";
        }
    }

    auto DescribeColor(const ColorTargetInfo& color, std::uint32_t count) -> std::string
    {
        if (!color.HasTarget) return std::format("rtvs {}, rtv0 none", count);

        return std::format("rtvs {}, rtv0 {}x{} res {} view {} back buffer {}", count, color.Width, color.Height,
            FormatText(color.ResourceFormat), color.ViewFormat != 0 ? FormatText(color.ViewFormat) : std::string{ "same" },
            color.IsBackBuffer ? "yes" : "no");
    }

    auto DescribeDepth(const DepthTargetInfo& depth) -> std::string
    {
        if (!depth.HasDepth) return "dsv none";

        return std::format("dsv {}x{} res {} view {} flags {} resource {:#x}", depth.Width, depth.Height, FormatText(depth.ResourceFormat),
            FormatText(depth.ViewFormat), depth.ViewFlags, depth.Resource);
    }

    // return: The first byte of the function of that caller, 0 for Other.
    auto CallerRvaOf(FirstPersonCaller caller) -> std::uint32_t
    {
        for (const auto& range : k_FirstPersonCallerRanges)
        {
            if (range.Caller == caller) return range.Rva;
        }

        return 0;
    }

    auto DescribeFirstPerson(const FirstPersonSample& sample) -> std::string
    {
        const std::string caller{ sample.Caller == FirstPersonCaller::Other ? std::format("other rva {:#X}", sample.CallerRva)
            : std::format("{:#X}", CallerRvaOf(sample.Caller)) };

        if (!sample.HasSample) return std::format("caller {} calls {} (no sample)", caller, sample.CallsPerFrame);
        if (!sample.IsSameThread) return std::format("caller {} calls {} pass {} (other thread, no D3D read)", caller, sample.CallsPerFrame, sample.Pass);

        const ViewportInfo& viewport{ sample.Viewport };
        const DepthStencilStateInfo& state{ sample.DepthState };
        const RasterizerStateInfo& raster{ sample.Rasterizer };

        std::string depthState{};

        if (state.HasState)
        {
            depthState = std::format("depth {} write {} func {} stencil {} rmask {:#X} wmask {:#X} ref {} front func {} pass {} fail {} dfail {}",
                state.IsDepthEnabled ? "on" : "off", state.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL ? "ALL" : "ZERO",
                ComparisonName(state.DepthFunc), state.IsStencilEnabled ? "on" : "off", state.StencilReadMask, state.StencilWriteMask,
                state.StencilRef, ComparisonName(state.FrontFunc), StencilOpName(state.FrontPassOp), StencilOpName(state.FrontFailOp),
                StencilOpName(state.FrontDepthFailOp));
        }
        else
        {
            depthState = std::format("depth state default (ref {})", state.StencilRef);
        }

        const std::string rasterizer{ raster.HasState ? std::format("bias {} slope {} clip {} cull {}", raster.DepthBias,
            raster.SlopeScaledDepthBias, raster.IsDepthClipEnabled ? "on" : "off", CullName(raster.CullMode)) : std::string{ "default" } };

        return std::format("caller {} calls {} pass {} | {} | {} | viewport {},{} {}x{} depth {}-{} | {} | rasterizer {}", caller,
            sample.CallsPerFrame, sample.Pass, DescribeColor(sample.Color, sample.RenderTargetCount), DescribeDepth(sample.Depth),
            viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth, depthState, rasterizer);
    }

    auto DescribeChud(const HudHookInfo& chud, const FirstPersonSample& view) -> std::string
    {
        if (!chud.HasSample) return "chud draw no sample";
        if (!chud.IsSameThread) return std::format("chud draw calls {} (other thread, no D3D read)", chud.CallsPerFrame);

        const std::string target{ chud.HasTarget ? std::format("rtv0 {}x{} view {} back buffer {}", chud.Width, chud.Height,
            FormatText(chud.Format), chud.IsBackBuffer ? "yes" : "no") : std::string{ "rtv0 none" } };

        std::string same{ "no view sample" };
        if (view.HasSample && view.IsSameThread && view.Depth.HasDepth && chud.Depth.HasDepth)
        {
            same = view.Depth.Resource == chud.Depth.Resource ? "yes" : "no";
        }
        else if (view.HasSample && view.IsSameThread && !chud.Depth.HasDepth)
        {
            same = "no dsv bound";
        }

        return std::format("chud draw calls {} | {} | {} | same dsv resource as 0x26EA78: {}", chud.CallsPerFrame, target,
            DescribeDepth(chud.Depth), same);
    }
}

namespace Engine::Render::System
{
    auto RenderService::OnInitialized(FrameHandler handler) -> void
    {
        m_OnInitialized.push_back(std::move(handler));
    }

    auto RenderService::OnFrame(FrameHandler handler) -> void
    {
        m_OnFrame.push_back(std::move(handler));
    }

    auto RenderService::OnPostFrame(FrameHandler handler) -> void
    {
        m_OnPostFrame.push_back(std::move(handler));
    }

    auto RenderService::OnResize(FrameHandler handler) -> void
    {
        m_OnResize.push_back(std::move(handler));
    }

    auto RenderService::OnBeforeHud(BeforeHudHandler handler) -> void
    {
        m_OnBeforeHud.push_back(std::move(handler));
    }

    auto RenderService::OnShutdown(ShutdownCallback callback) -> void
    {
        m_OnShutdown.push_back(std::move(callback));
    }

    auto RenderService::PresentFrame(IDXGISwapChain* swapChain) -> void
    {
        if (m_RenderStore.IsResizing()) return;

        if (m_RenderStore.IsShutdownRequested())
        {
            this->Shutdown();
            return;
        }

        if (m_RenderStore.StepFirstPersonProbe()) this->ReportFirstPersonProbe();
        else
        {
            this->PrepareDepthPairs();
            this->PrepareColorPairs();
        }

        this->PrepareMask();

        // A different swap chain than the bound one means the game created a new one.
        const bool isAnotherSwapChain = m_RenderStore.GetSwapChain() != swapChain;
        if (!m_RenderStore.IsReady() || isAnotherSwapChain)
        {
            this->Bind(swapChain);
            if (!m_RenderStore.IsReady()) return;
        }

        this->Raise(m_OnFrame, "OnFrame");
        this->Raise(m_OnPostFrame, "OnPostFrame");
    }

    auto RenderService::BeginResize() -> void
    {
        m_RenderStore.SetResizing(true);

        if (auto* context = m_RenderStore.GetContext())
        {
            context->OMSetRenderTargets(0, nullptr, nullptr);
        }

        m_RenderStore.ReleaseTargets();
    }

    auto RenderService::EndResize(IDXGISwapChain* swapChain, HRESULT result) -> void
    {
        if (SUCCEEDED(result) && !m_RenderStore.IsShutdownRequested())
        {
            this->Bind(swapChain);
        }

        m_RenderStore.SetResizing(false);
    }

    auto RenderService::Bind(IDXGISwapChain* swapChain) -> void
    {
        const bool isFirstBind = m_RenderStore.GetSwapChain() != swapChain;

        if (!this->Attach(swapChain)) return;

        this->Raise(isFirstBind ? m_OnInitialized : m_OnResize,
            isFirstBind ? "OnInitialized" : "OnResize");
    }

    auto RenderService::Attach(IDXGISwapChain* swapChain) -> bool
    {
        auto fail = [this](const char* reason)
        {
            if (!m_IsAttachFailed)
            {
                m_Log.Message("[RenderService] ERROR: {}", reason);
                m_IsAttachFailed = true;
            }

            return false;
        };

        ComPtr<ID3D11Device> device{};
        if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(device.GetAddressOf()))))
        {
            return fail("The swap chain does not belong to a D3D11 device.");
        }

        ComPtr<ID3D11DeviceContext> context{};
        device->GetImmediateContext(context.GetAddressOf());

        DXGI_SWAP_CHAIN_DESC swapChainDesc{};
        if (FAILED(swapChain->GetDesc(&swapChainDesc)))
        {
            return fail("IDXGISwapChain::GetDesc failed.");
        }

        ComPtr<ID3D11Texture2D> backBuffer{};
        if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()))))
        {
            return fail("IDXGISwapChain::GetBuffer failed.");
        }

        D3D11_TEXTURE2D_DESC bufferDesc{};
        backBuffer->GetDesc(&bufferDesc);

        ComPtr<ID3D11RenderTargetView> renderTarget{};
        if (FAILED(device->CreateRenderTargetView(backBuffer.Get(), nullptr,
            renderTarget.GetAddressOf())))
        {
            return fail("CreateRenderTargetView failed.");
        }

        // Depth buffer with the size and samples of the back buffer.
        D3D11_TEXTURE2D_DESC depthDesc{};
        depthDesc.Width = bufferDesc.Width;
        depthDesc.Height = bufferDesc.Height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
        depthDesc.SampleDesc = bufferDesc.SampleDesc;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ComPtr<ID3D11DepthStencilView> depthStencil{};
        ComPtr<ID3D11Texture2D> depthTexture{};

        if (SUCCEEDED(device->CreateTexture2D(&depthDesc, nullptr,
            depthTexture.GetAddressOf())))
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc{};
            viewDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            viewDesc.ViewDimension = bufferDesc.SampleDesc.Count > 1 ?
                D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;

            if (FAILED(device->CreateDepthStencilView(depthTexture.Get(),
                &viewDesc, depthStencil.GetAddressOf())))
            {
                m_Log.Message("[RenderService] ERROR:"
                    " CreateDepthStencilView failed.");
            }
        }
        else
        {
            m_Log.Message("[RenderService] ERROR:"
                " Depth CreateTexture2D failed.");
        }

        m_RenderStore.SetDevice(std::move(device), std::move(context),
            swapChainDesc.OutputWindow, swapChain);

        m_RenderStore.SetTargets(std::move(renderTarget), std::move(depthStencil),
            bufferDesc.Width, bufferDesc.Height);

        m_IsAttachFailed = false;
        return true;
    }

    auto RenderService::MakeMask(bool isInFrame) const -> MaskContext
    {
        MaskContext mask{};

        if (!m_RenderStore.IsFirstPersonMaskRequested()) return mask;

        const bool isScene{ m_RenderStore.IsOverlayInSceneRequested() };

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i)
        {
            const MaskPairKind kind{ static_cast<MaskPairKind>(i) };

            // The color pair is not used (nor copied) when the overlay draws in the scene: the shader leaves it out.
            if (!IsMaskPairWanted(true, isScene, kind) || !m_RenderStore.AreMaskTexturesReady(kind)) continue;

            const D3D11_TEXTURE2D_DESC desc{ m_RenderStore.GetMaskDesc(kind) };
            MaskPairView& pair{ mask.Pairs[i] };

            pair.IsValid = isInFrame ? m_RenderStore.IsMaskCopied(kind) : m_RenderStore.WasMaskCopiedLastFrame(kind);
            pair.Pre = m_RenderStore.GetMaskPreView(kind);
            pair.Post = m_RenderStore.GetMaskPostView(kind);
            pair.Width = desc.Width;
            pair.Height = desc.Height;
        }

        return mask;
    }

    auto RenderService::MakeContext(bool isInFrame) const -> FrameContext
    {
        FrameContext frame{};
        frame.Device = m_RenderStore.GetDevice();
        frame.Context = m_RenderStore.GetContext();
        frame.RenderTarget = m_RenderStore.GetRenderTarget();
        frame.DepthStencil = m_RenderStore.GetDepthStencil();
        frame.Window = m_RenderStore.GetWindow();
        frame.Width = m_RenderStore.GetWidth();
        frame.Height = m_RenderStore.GetHeight();
        frame.Mask = this->MakeMask(isInFrame);
        return frame;
    }

    auto RenderService::Raise(const std::vector<FrameHandler>& handlers,
        const char* stage) -> void
    {
        const FrameContext frame = this->MakeContext(false);

        for (const auto& handler : handlers)
        {
            try
            {
                handler(frame);
            }
            catch (...)
            {
                this->ReportHandlerError(stage);
            }
        }
    }

    auto RenderService::ReportHandlerError(const char* stage) noexcept -> void
    {
        if (m_HandlerErrors.fetch_add(1) >= 5) return;

        try
        {
            m_Log.Message("[RenderService] ERROR: A {} handler threw an exception.", stage);
        }
        catch (...)
        {
        }
    }

    auto RenderService::RequestShutdown() -> void
    {
        m_RenderStore.RequestShutdown();
    }

    auto RenderService::WaitForShutdown(milliseconds timeout) -> bool
    {
        const auto deadline = steady_clock::now() + timeout;

        while (!m_RenderStore.IsShutdownDone())
        {
            if (steady_clock::now() >= deadline) return false;
            std::this_thread::sleep_for(milliseconds{ 1 });
        }

        return true;
    }

    auto RenderService::FindBeforeHudBlocker(const HudHookInfo& sample, bool isReady, std::uint32_t width, std::uint32_t height)
        -> std::optional<BeforeHudStatus>
    {
        return FindTargetBlocker(sample, isReady, width, height, false);
    }

    auto RenderService::FindSceneBlocker(const HudHookInfo& sample, bool isReady, std::uint32_t width, std::uint32_t height)
        -> std::optional<BeforeHudStatus>
    {
        return FindTargetBlocker(sample, isReady, width, height, true);
    }

    auto RenderService::OnHudDraw() -> void
    {
        try
        {
            this->HandleHudDraw();
        }
        catch (...)
        {
            m_RenderStore.CountHudFailure();
        }
    }

    auto RenderService::HandleHudDraw() -> void
    {
        m_RenderStore.CountHudCall();

        const HudHookInfo sample{ this->MeasureHud() };

        if (sample.IsSameThread) this->ReadFirstPersonStencil();

        if (m_RenderStore.IsGameHudHidden())
        {
            m_RenderStore.SetBeforeHudStatus(BeforeHudStatus::HudHidden);
            return;
        }

        // The handlers run once per Present frame, whatever the number of HUD draws.
        if (m_RenderStore.IsBeforeHudMarked()) return;

        const bool isReady{ m_RenderStore.IsReady() && !m_RenderStore.IsResizing() && !m_RenderStore.IsShutdownRequested() };

        if (const auto blocker{ this->FindBeforeHudBlocker(sample, isReady, m_RenderStore.GetWidth(), m_RenderStore.GetHeight()) })
        {
            m_RenderStore.SetBeforeHudStatus(*blocker);
            return;
        }

        ID3D11DeviceContext* context{ m_RenderStore.GetContext() };

        ComPtr<ID3D11RenderTargetView> target{};
        if (context != nullptr) context->OMGetRenderTargets(1, target.GetAddressOf(), nullptr);

        if (context == nullptr || target == nullptr)
        {
            m_RenderStore.SetBeforeHudStatus(BeforeHudStatus::NotReady);
            return;
        }

        FrameContext frame{ this->MakeContext(true) };
        frame.RenderTarget = target.Get();
        frame.Width = sample.Width;
        frame.Height = sample.Height;

        const BeforeHudStatus status{ this->RunBeforeHudHandlers(frame, context) };

        if (status != BeforeHudStatus::Declined) m_RenderStore.SetDrawPlace(DrawPlace::Hud);
        m_RenderStore.SetBeforeHudStatus(status);
    }

    auto RenderService::RunBeforeHudHandlers(const FrameContext& frame, ID3D11DeviceContext* context) -> BeforeHudStatus
    {
        bool isDrawn{ false };
        bool isFailed{ false };

        {
            GpuStateGuard guard{ context };

            for (const auto& handler : m_OnBeforeHud)
            {
                try
                {
                    if (handler(frame)) isDrawn = true;
                }
                catch (...)
                {
                    // The handler may have drawn part of the frame: it counts as handled, so nobody draws it again.
                    isFailed = true;
                    this->ReportHandlerError("OnBeforeHud");
                }
            }
        }

        if (isDrawn || isFailed) m_RenderStore.MarkBeforeHud();
        return isFailed ? BeforeHudStatus::Failed : isDrawn ? BeforeHudStatus::Drawn : BeforeHudStatus::Declined;
    }

    auto RenderService::OnSceneDraw() -> void
    {
        try
        {
            this->HandleSceneDraw();
        }
        catch (...)
        {
            m_RenderStore.CountSceneFailure();
        }
    }

    auto RenderService::ComputeSceneColorScale(bool isByGameHdr, float hdrScale, float slider) -> float
    {
        const float factor{ std::isfinite(slider) && slider > 0.0f ? slider : 1.0f };
        if (!isByGameHdr || !std::isfinite(hdrScale) || hdrScale <= 0.0f) return factor;

        return hdrScale * factor;
    }

    auto RenderService::HandleSceneDraw() -> void
    {
        if (!m_RenderStore.IsOverlayInSceneRequested() || m_RenderStore.IsBeforeHudMarked()) return;

        const HudHookInfo sample{ this->MeasureBoundTarget() };

        const bool isReady{ m_RenderStore.IsReady() && !m_RenderStore.IsResizing() && !m_RenderStore.IsShutdownRequested() };

        if (const auto blocker{ RenderService::FindSceneBlocker(sample, isReady, m_RenderStore.GetWidth(), m_RenderStore.GetHeight()) })
        {
            m_RenderStore.SetSceneStatus(*blocker);
            return;
        }

        // Only a call that passes the blocker (so on the Present thread) takes the point of the frame: a blocked call of another view
        // does not use it up, and a later call of the main view can still draw.
        if (!m_RenderStore.TakeSceneCall()) return;

        ID3D11DeviceContext* context{ m_RenderStore.GetContext() };

        ComPtr<ID3D11RenderTargetView> target{};
        if (context != nullptr) context->OMGetRenderTargets(1, target.GetAddressOf(), nullptr);

        if (context == nullptr || target == nullptr)
        {
            m_RenderStore.SetSceneStatus(BeforeHudStatus::NotReady);
            return;
        }

        FrameContext frame{ this->MakeContext(true) };
        frame.RenderTarget = target.Get();
        frame.Width = sample.Width;
        frame.Height = sample.Height;

        // note: The factors were read by the hook of the same call, a moment ago. Only the scene point scales the color.
        frame.ColorScale = RenderService::ComputeSceneColorScale(m_RenderStore.IsSceneColorByGameHdr(), m_RenderStore.GetSceneColorSample().HdrScale,
            m_RenderStore.GetSceneColorSlider());
        m_RenderStore.SetSceneColorScale(frame.ColorScale);

        const BeforeHudStatus status{ this->RunBeforeHudHandlers(frame, context) };
        m_RenderStore.SetSceneStatus(status);

        if (status == BeforeHudStatus::Declined) return;

        m_RenderStore.SetDrawPlace(DrawPlace::Scene);

        if (!m_IsSceneLogged)
        {
            const char* format{ FormatName(sample.Format) };
            m_IsSceneLogged = true;
            m_Log.Message("[RenderService] INFO: The overlay draws in the scene, target format {} ({}), {}x{}.", format != nullptr ? format : "unknown", sample.Format, sample.Width, sample.Height);
        }
    }

    auto RenderService::MeasureHud() -> HudHookInfo
    {
        const HudHookInfo sample{ this->MeasureBoundTarget() };
        m_RenderStore.SetHudSample(sample);
        return sample;
    }

    auto RenderService::MeasureBoundTarget() -> HudHookInfo
    {
        HudHookInfo sample{};
        sample.HasSample = true;

        const std::uint32_t presentThread{ m_RenderStore.GetPresentThread() };
        sample.IsSameThread = presentThread != 0 && presentThread == GetCurrentThreadId();

        if (!sample.IsSameThread || m_RenderStore.IsResizing() || !m_RenderStore.IsReady())
        {
            return sample;
        }

        ID3D11DeviceContext* context{ m_RenderStore.GetContext() };

        if (context == nullptr)
        {
            return sample;
        }

        ComPtr<ID3D11RenderTargetView> target{};
        ComPtr<ID3D11DepthStencilView> depth{};
        context->OMGetRenderTargets(1, target.GetAddressOf(), depth.GetAddressOf());

        const ColorTargetInfo color{ MeasureColorTarget(target.Get(), m_RenderStore.GetRenderTarget(), m_RenderStore.GetDevice()) };

        if (color.HasTarget)
        {
            sample.HasTarget = true;
            sample.Width = color.Width;
            sample.Height = color.Height;
            sample.SampleCount = color.SampleCount;
            sample.Format = color.ViewFormat != DXGI_FORMAT_UNKNOWN ? color.ViewFormat : color.ResourceFormat;
            sample.IsBackBuffer = color.IsBackBuffer;
            sample.IsSameDevice = color.IsSameDevice;
        }

        sample.Depth = MeasureDepthTarget(depth.Get());

        UINT viewportCount{ 1 };
        D3D11_VIEWPORT viewport{};
        context->RSGetViewports(&viewportCount, &viewport);

        if (viewportCount >= 1 && viewport.Width > 0.0f)
        {
            sample.HasViewport = true;
            sample.ViewportX = viewport.TopLeftX;
            sample.ViewportY = viewport.TopLeftY;
            sample.ViewportWidth = viewport.Width;
            sample.ViewportHeight = viewport.Height;
        }

        return sample;
    }

    auto RenderService::MeasureFirstPerson(FirstPersonCaller caller, std::uint32_t callerRva, std::uint8_t pass) -> void
    {
        FirstPersonSample sample{};
        sample.HasSample = true;
        sample.Caller = caller;
        sample.CallerRva = callerRva;
        sample.Pass = pass;

        const std::uint32_t presentThread{ m_RenderStore.GetPresentThread() };
        sample.IsSameThread = presentThread != 0 && presentThread == GetCurrentThreadId();

        ID3D11DeviceContext* context{ sample.IsSameThread && !m_RenderStore.IsResizing() && m_RenderStore.IsReady()
            ? m_RenderStore.GetContext() : nullptr };

        if (context == nullptr)
        {
            // Another thread, resizing or not ready: nothing of D3D is touched and the sample only says so.
            m_RenderStore.SetFirstPersonSample(sample);
            return;
        }

        std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> views{};
        ComPtr<ID3D11DepthStencilView> depth{};
        context->OMGetRenderTargets(static_cast<UINT>(views.size()), views.data(), depth.GetAddressOf());

        ComPtr<ID3D11RenderTargetView> first{};

        for (std::size_t i{ 0 }; i < views.size(); ++i)
        {
            if (views[i] == nullptr) continue;

            ++sample.RenderTargetCount;

            if (i == 0) first.Attach(views[i]);
            else views[i]->Release();
        }

        sample.Color = MeasureColorTarget(first.Get(), m_RenderStore.GetRenderTarget(), m_RenderStore.GetDevice());
        sample.Depth = MeasureDepthTarget(depth.Get());

        if (depth != nullptr && (caller == FirstPersonCaller::Pass1 || caller == FirstPersonCaller::Func26E2A0))
        {
            // The depth stencil of the model draw is kept (one reference) to read its stencil at the HUD draw.
            ComPtr<ID3D11Resource> depthResource{};
            depth->GetResource(depthResource.GetAddressOf());
            m_RenderStore.CaptureFirstPersonDepth(depthResource.Get());
        }

        // Measured (VW.6.y.2 A2): at the HUD draw the depth stencil no longer holds the first person model (no 0x80 bit, far depth only,
        // the same with the model hidden). The forward phase of the view comes after the G-buffer passes and before the post, so the
        // stencil is read there (once per probe, the HUD draw then finds it attempted). The view is bound read only, a copy only reads it.
        if (caller == FirstPersonCaller::ViewPhase) this->ReadFirstPersonStencil();

        UINT viewportCount{ 1 };
        D3D11_VIEWPORT viewport{};
        context->RSGetViewports(&viewportCount, &viewport);

        if (viewportCount >= 1)
        {
            sample.Viewport = { .HasViewport = true, .X = viewport.TopLeftX, .Y = viewport.TopLeftY, .Width = viewport.Width,
                .Height = viewport.Height, .MinDepth = viewport.MinDepth, .MaxDepth = viewport.MaxDepth };
        }

        ComPtr<ID3D11DepthStencilState> depthState{};
        UINT stencilRef{ 0 };
        context->OMGetDepthStencilState(depthState.GetAddressOf(), &stencilRef);
        sample.DepthState.StencilRef = stencilRef;

        if (depthState != nullptr)
        {
            D3D11_DEPTH_STENCIL_DESC desc{};
            depthState->GetDesc(&desc);

            sample.DepthState.HasState = true;
            sample.DepthState.IsDepthEnabled = desc.DepthEnable != FALSE;
            sample.DepthState.IsStencilEnabled = desc.StencilEnable != FALSE;
            sample.DepthState.DepthWriteMask = desc.DepthWriteMask;
            sample.DepthState.DepthFunc = desc.DepthFunc;
            sample.DepthState.StencilReadMask = desc.StencilReadMask;
            sample.DepthState.StencilWriteMask = desc.StencilWriteMask;
            sample.DepthState.FrontFunc = desc.FrontFace.StencilFunc;
            sample.DepthState.FrontPassOp = desc.FrontFace.StencilPassOp;
            sample.DepthState.FrontFailOp = desc.FrontFace.StencilFailOp;
            sample.DepthState.FrontDepthFailOp = desc.FrontFace.StencilDepthFailOp;
        }

        ComPtr<ID3D11RasterizerState> rasterizer{};
        context->RSGetState(rasterizer.GetAddressOf());

        if (rasterizer != nullptr)
        {
            D3D11_RASTERIZER_DESC desc{};
            rasterizer->GetDesc(&desc);

            sample.Rasterizer.HasState = true;
            sample.Rasterizer.IsDepthClipEnabled = desc.DepthClipEnable != FALSE;
            sample.Rasterizer.DepthBias = desc.DepthBias;
            sample.Rasterizer.SlopeScaledDepthBias = desc.SlopeScaledDepthBias;
            sample.Rasterizer.CullMode = desc.CullMode;
        }

        m_RenderStore.SetFirstPersonSample(sample);
    }

    auto RenderService::GetPresentContext() -> ID3D11DeviceContext*
    {
        if (m_RenderStore.IsResizing() || !m_RenderStore.IsReady()) return nullptr;

        const std::uint32_t presentThread{ m_RenderStore.GetPresentThread() };
        if (presentThread == 0 || presentThread != GetCurrentThreadId()) return nullptr;

        return m_RenderStore.GetContext();
    }

    auto RenderService::GetDepthPairContext() -> ID3D11DeviceContext*
    {
        if (!m_RenderStore.IsFirstPersonProbeArmed() || !m_RenderStore.GetDepthPairs().AreReady()) return nullptr;

        return this->GetPresentContext();
    }

    auto RenderService::PlanMask(bool isRequested, const std::optional<D3D11_TEXTURE2D_DESC>& wanted, bool isAttempted,
        const D3D11_TEXTURE2D_DESC& attempted) -> MaskPlan
    {
        if (!isRequested) return isAttempted ? MaskPlan::Release : MaskPlan::Keep;
        if (!wanted) return MaskPlan::Keep;
        if (!isAttempted || !IsSameTextureDesc(*wanted, attempted)) return MaskPlan::Create;

        return MaskPlan::Keep;
    }

    auto RenderService::ScaleToMask(std::uint32_t pixel, std::uint32_t targetSize, std::uint32_t maskSize) -> std::uint32_t
    {
        if (targetSize == 0 || maskSize == 0) return 0;

        const std::uint64_t texel{ static_cast<std::uint64_t>(pixel) * maskSize / targetSize };
        return static_cast<std::uint32_t>((std::min)(texel, static_cast<std::uint64_t>(maskSize - 1)));
    }

    auto RenderService::BeginMaskPairCopy(MaskPairKind kind, ID3D11DeviceContext* context, ID3D11Resource* resource, bool canNote) -> bool
    {
        ComPtr<ID3D11Texture2D> texture{};
        if (resource != nullptr) resource->QueryInterface(IID_PPV_ARGS(texture.GetAddressOf()));
        if (texture == nullptr) return false;

        D3D11_TEXTURE2D_DESC desc{};
        texture->GetDesc(&desc);

        if (!m_RenderStore.AreMaskTexturesReady(kind) || !IsSameTextureDesc(desc, m_RenderStore.GetMaskDesc(kind)))
        {
            if (canNote) m_RenderStore.NoteMaskDesc(kind, desc);
            return false;
        }

        // The copy of the frame is taken only once a resource of the expected description is bound, so a call without one (another view) leaves
        // it to the next call of the same frame.
        if (!m_RenderStore.TakeMaskCopy(kind)) return false;

        m_RenderStore.SetMaskSource(kind, resource);
        context->CopyResource(m_RenderStore.GetMaskPre(kind), resource);
        return true;
    }

    auto RenderService::BeginMaskCopy(std::int32_t pass) -> std::optional<MaskPairKind>
    {
        if (!m_RenderStore.IsFirstPersonMaskRequested()) return std::nullopt;

        MaskPairKind kind{ MaskPairKind::DepthB };

        if (pass == k_FirstPersonMaskPass)
        {
            // Counted before any check that can leave without copying, so the telemetry sees every call.
            m_RenderStore.CountMaskPassCall();
        }
        else if (pass == k_FirstPersonZeroPass) kind = MaskPairKind::DepthZero;
        else return std::nullopt;

        ID3D11DeviceContext* context{ this->GetPresentContext() };
        if (context == nullptr) return std::nullopt;

        ComPtr<ID3D11Resource> resource{ BoundDepthResource(context) };

        // The pass 0 call can run with no depth stencil bound (the game binds its own inside): the one of the pass 0xB call of the frame is the
        // same resource for the whole view.
        if (kind == MaskPairKind::DepthZero)
        {
            ComPtr<ID3D11Resource> saved{ m_RenderStore.GetMaskFrameDepth() };

            switch (ChooseZeroMaskSource(resource != nullptr, saved != nullptr))
            {
            case MaskSource::Bound:
                break;

            case MaskSource::Saved:
                resource = std::move(saved);
                break;

            case MaskSource::None:
                return std::nullopt;
            }
        }

        // The pass 0 pair is made from the description of the pass 0xB pair, so it never notes one.
        if (!this->BeginMaskPairCopy(kind, context, resource.Get(), kind == MaskPairKind::DepthB)) return std::nullopt;

        if (kind == MaskPairKind::DepthB) m_RenderStore.SetMaskFrameDepth(resource.Get());

        return kind;
    }

    auto RenderService::EndMaskCopy(MaskPairKind kind) -> void
    {
        ID3D11DeviceContext* context{ this->GetPresentContext() };
        ID3D11Resource* source{ m_RenderStore.GetMaskSource(kind) };
        ID3D11Texture2D* post{ m_RenderStore.GetMaskPost(kind) };

        if (context != nullptr && source != nullptr && post != nullptr)
        {
            context->CopyResource(post, source);
            m_RenderStore.MarkMaskCopied(kind);
        }

        m_RenderStore.SetMaskSource(kind, nullptr);
    }

    auto RenderService::BeginColorMaskCopy(std::uint8_t phase) -> bool
    {
        if (phase == k_ForwardDepthPhase ||
            !IsMaskPairWanted(m_RenderStore.IsFirstPersonMaskRequested(), m_RenderStore.IsOverlayInSceneRequested(), MaskPairKind::Color)) return false;

        ID3D11DeviceContext* context{ this->GetPresentContext() };
        if (context == nullptr) return false;

        ComPtr<ID3D11Resource> resource{ BoundColorResource(context) };
        return this->BeginMaskPairCopy(MaskPairKind::Color, context, resource.Get(), true);
    }

    auto RenderService::EndColorMaskCopy() -> void
    {
        this->EndMaskCopy(MaskPairKind::Color);
    }

    auto RenderService::PrepareMask() -> void
    {
        if (m_RenderStore.IsResizing() || !m_RenderStore.IsReady()) return;

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i) this->PrepareMaskPair(static_cast<MaskPairKind>(i));
    }

    auto RenderService::PrepareMaskPair(MaskPairKind kind) -> void
    {
        const bool isDepth{ kind != MaskPairKind::Color };
        const std::optional<D3D11_TEXTURE2D_DESC> wanted{ m_RenderStore.GetMaskWantedDesc(kind == MaskPairKind::DepthZero ? MaskPairKind::DepthB : kind) };

        switch (RenderService::PlanMask(IsMaskPairWanted(m_RenderStore.IsFirstPersonMaskRequested(), m_RenderStore.IsOverlayInSceneRequested(), kind), wanted, m_RenderStore.IsMaskAttempted(kind),
            m_RenderStore.GetMaskDesc(kind)))
        {
        case MaskPlan::Keep:
            return;

        case MaskPlan::Release:
            m_RenderStore.ReleaseMaskTextures(kind);
            return;

        case MaskPlan::Create:
            // A pair copied in the last frame keeps its textures: another call of the frame with a different target (another view or phase)
            // notes its description too, and recreating from it would flip the textures every frame and never copy.
            if (m_RenderStore.WasMaskCopiedLastFrame(kind)) return;
            break;
        }

        // The depth is read as R32_FLOAT_X8X24 and the color in its own format, both without multisampling: another texture is attempted once
        // and logged.
        const std::optional<DXGI_FORMAT> viewFormat{ isDepth ? std::optional<DXGI_FORMAT>{ DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS }
            : ColorMaskViewFormat(wanted->Format) };
        const bool isSupported{ isDepth ? wanted->Format == DXGI_FORMAT_R32G8X24_TYPELESS : viewFormat.has_value() };

        if (!isSupported || wanted->SampleDesc.Count != 1)
        {
            m_RenderStore.SetMaskTextures(kind, {}, {}, {}, {}, *wanted);
            m_Log.Message("[RenderService] WARNING: The {} texture of the first person mask cannot be a source (format {}, samples {}).",
                MaskPairName(kind), static_cast<std::uint32_t>(wanted->Format), wanted->SampleDesc.Count);
            return;
        }

        MaskCreation creation{ CreateMaskTextures(m_RenderStore.GetDevice(), *wanted, *viewFormat) };

        if (FAILED(creation.Result))
        {
            m_RenderStore.SetMaskTextures(kind, {}, {}, {}, {}, *wanted);
            m_Log.Message("[RenderService] ERROR: Failed to create the first person mask textures ({}), hresult {:#010X}.", MaskPairName(kind),
                static_cast<std::uint32_t>(creation.Result));
            return;
        }

        m_RenderStore.SetMaskTextures(kind, std::move(creation.Textures[0]), std::move(creation.Textures[1]), std::move(creation.Views[0]),
            std::move(creation.Views[1]), *wanted);
        m_Log.Message("[RenderService] INFO: First person mask textures made ({}), {}x{}.", MaskPairName(kind), wanted->Width, wanted->Height);
    }

    auto RenderService::BeginDepthPair(FirstPersonCaller caller, std::int32_t pass) -> std::optional<std::size_t>
    {
        ID3D11DeviceContext* context{ this->GetDepthPairContext() };
        if (context == nullptr) return std::nullopt;

        ComPtr<ID3D11Resource> resource{ BoundDepthResource(context) };

        // The pass may be entered with no depth stencil bound: the game sets its own inside, so fall back to the one captured from the model.
        const bool isCaptured{ resource == nullptr };
        if (isCaptured) resource = m_RenderStore.GetFirstPersonDepth();

        return BeginPairCopy(context, m_RenderStore.GetDepthPairs(), resource.Get(),
            DepthPairNote{ .Caller = caller, .Pass = pass, .IsCaptured = isCaptured });
    }

    auto RenderService::EndDepthPair(std::size_t pair) -> void
    {
        EndPairCopy(this->GetDepthPairContext(), m_RenderStore.GetDepthPairs(), pair);
    }

    auto RenderService::BeginColorPair(std::uint8_t phase) -> std::optional<std::size_t>
    {
        if (!m_RenderStore.IsFirstPersonProbeArmed()) return std::nullopt;

        ID3D11DeviceContext* context{ this->GetPresentContext() };
        if (context == nullptr) return std::nullopt;

        ComPtr<ID3D11Resource> resource{ BoundColorResource(context) };
        CopyPairs<ColorPairNote, k_ColorPairCount>& pairs{ m_RenderStore.GetColorPairs() };

        if (!pairs.AreReady())
        {
            // Frame 1 of the probe: only the description is kept, Present makes the copies for the frame that follows.
            ComPtr<ID3D11Texture2D> texture{};
            if (resource != nullptr) resource.As(&texture);

            if (texture != nullptr)
            {
                D3D11_TEXTURE2D_DESC desc{};
                texture->GetDesc(&desc);
                m_RenderStore.NoteColorDesc(desc);
            }

            return std::nullopt;
        }

        ColorPairNote note{ .Phase = phase };

        if (resource == nullptr)
        {
            note.State = PairState::NoTarget;
            pairs.NoteCall(note, nullptr);
            return std::nullopt;
        }

        return BeginPairCopy(context, pairs, resource.Get(), note);
    }

    auto RenderService::EndColorPair(std::size_t pair) -> void
    {
        EndPairCopy(this->GetPresentContext(), m_RenderStore.GetColorPairs(), pair);
    }

    auto RenderService::CompareTexels(const std::uint8_t* before, std::uint32_t beforePitch, const std::uint8_t* after,
        std::uint32_t afterPitch, std::uint32_t width, std::uint32_t height, const TexelLayout& layout, ChangedMap* changedMap)
        -> std::optional<TexelDiff>
    {
        const std::uint64_t rowBytes{ static_cast<std::uint64_t>(width) * layout.TexelBytes };

        if (before == nullptr || after == nullptr || width == 0 || height == 0 || layout.TexelBytes == 0 || layout.CompareBytes == 0 ||
            layout.CompareBytes > layout.TexelBytes || (layout.HasDepthRange && layout.CompareBytes < sizeof(float)) ||
            beforePitch < rowBytes || afterPitch < rowBytes)
        {
            return std::nullopt;
        }

        if (changedMap != nullptr)
        {
            changedMap->Width = width;
            changedMap->Height = height;
            changedMap->Texels.assign(static_cast<std::size_t>(width) * height, 0);
        }

        TexelDiff diff{};

        for (std::uint32_t y{ 0 }; y < height; ++y)
        {
            const std::uint8_t* beforeRow{ before + static_cast<std::size_t>(y) * beforePitch };
            const std::uint8_t* afterRow{ after + static_cast<std::size_t>(y) * afterPitch };

            for (std::uint32_t x{ 0 }; x < width; ++x)
            {
                const std::uint8_t* beforeTexel{ beforeRow + static_cast<std::size_t>(x) * layout.TexelBytes };
                const std::uint8_t* afterTexel{ afterRow + static_cast<std::size_t>(x) * layout.TexelBytes };

                if (std::memcmp(beforeTexel, afterTexel, layout.CompareBytes) == 0) continue;

                float depth{ 0.0f };
                if (layout.HasDepthRange) std::memcpy(&depth, afterTexel, sizeof(depth));

                if (diff.Changed == 0)
                {
                    diff.X0 = diff.X1 = x;
                    diff.Y0 = diff.Y1 = y;
                    diff.MinDepth = diff.MaxDepth = depth;
                }
                else
                {
                    diff.X0 = (std::min)(diff.X0, x);
                    diff.X1 = (std::max)(diff.X1, x);
                    diff.Y0 = (std::min)(diff.Y0, y);
                    diff.Y1 = (std::max)(diff.Y1, y);
                    diff.MinDepth = (std::min)(diff.MinDepth, depth);
                    diff.MaxDepth = (std::max)(diff.MaxDepth, depth);
                }

                ++diff.Changed;
                if (IsInStencilRegion(StencilRegion::Weapon, x, y, width, height)) ++diff.Weapon;
                if (x >= width / 2) ++diff.Right;
                if (changedMap != nullptr) changedMap->Texels[static_cast<std::size_t>(y) * width + x] = 1;
            }
        }

        return diff;
    }

    auto RenderService::CompareDepth(const std::uint8_t* before, std::uint32_t beforePitch, const std::uint8_t* after,
        std::uint32_t afterPitch, std::uint32_t width, std::uint32_t height) -> std::optional<TexelDiff>
    {
        return RenderService::CompareTexels(before, beforePitch, after, afterPitch, width, height, k_DepthTexelLayout, nullptr);
    }

    auto RenderService::CountOutsideMask(const ChangedMap& depth, const ChangedMap& color) -> std::optional<std::uint32_t>
    {
        const std::size_t texels{ static_cast<std::size_t>(color.Width) * color.Height };

        if (texels == 0 || depth.Width != color.Width || depth.Height != color.Height || color.Texels.size() != texels ||
            depth.Texels.size() != texels)
        {
            return std::nullopt;
        }

        std::uint32_t outside{ 0 };

        for (std::size_t i{ 0 }; i < texels; ++i)
        {
            if (color.Texels[i] != 0 && depth.Texels[i] == 0) ++outside;
        }

        return outside;
    }

    auto RenderService::PrepareDepthPairs() -> void
    {
        if (!m_RenderStore.IsFirstPersonProbeArmed() || m_RenderStore.GetDepthPairs().GetSetup().IsAttempted) return;
        if (m_RenderStore.IsResizing() || !m_RenderStore.IsReady()) return;

        ID3D11Resource* resource{ m_RenderStore.GetFirstPersonDepth() };
        if (resource == nullptr) return;

        CopyPairSetup setup{ .IsAttempted = true };

        ComPtr<ID3D11Texture2D> texture{};

        if (FAILED(resource->QueryInterface(IID_PPV_ARGS(texture.GetAddressOf()))))
        {
            setup.Status = StencilReadStatus::NotTexture;
            m_RenderStore.GetDepthPairs().SetSetup(setup);
            return;
        }

        D3D11_TEXTURE2D_DESC desc{};
        texture->GetDesc(&desc);

        if (desc.Format != DXGI_FORMAT_R32G8X24_TYPELESS && desc.Format != DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
        {
            setup.Status = StencilReadStatus::UnsupportedFormat;
            m_RenderStore.GetDepthPairs().SetSetup(setup);
            return;
        }

        ComPtr<ID3D11Device> device{};
        resource->GetDevice(device.GetAddressOf());

        m_RenderStore.GetDepthPairs().SetSetup(CreateCopyPairs(device.Get(), desc, m_RenderStore.GetDepthPairs()));
    }

    auto RenderService::PrepareColorPairs() -> void
    {
        if (!m_RenderStore.IsFirstPersonProbeArmed() || m_RenderStore.GetColorPairs().GetSetup().IsAttempted) return;
        if (m_RenderStore.IsResizing() || !m_RenderStore.IsReady()) return;

        const std::optional<D3D11_TEXTURE2D_DESC> wanted{ m_RenderStore.GetColorDesc() };
        if (!wanted) return;

        // The copies are compared by 4 bytes per texel and read without multisampling: another target is attempted once and reported.
        if (wanted->SampleDesc.Count != 1 || ColorTexelBytes(wanted->Format) == 0)
        {
            m_RenderStore.GetColorPairs().SetSetup(CopyPairSetup{ .IsAttempted = true, .Status = StencilReadStatus::UnsupportedFormat });
            return;
        }

        m_RenderStore.GetColorPairs().SetSetup(CreateCopyPairs(m_RenderStore.GetDevice(), *wanted, m_RenderStore.GetColorPairs()));
    }

    auto RenderService::ReadDepthPairs(ChangedMap& mask) -> DepthProbeInfo
    {
        const CopyPairs<DepthPairNote, k_DepthPairCount>& pairs{ m_RenderStore.GetDepthPairs() };
        DepthProbeInfo info{ .Count = (std::min)(pairs.GetNoteCount(), static_cast<std::uint32_t>(k_DepthPairCount)) };

        ID3D11Texture2D* staging{ pairs.GetStaging() };
        if (staging == nullptr) return info;

        const D3D11_TEXTURE2D_DESC desc{ pairs.GetDesc() };
        const std::uint32_t pitch{ desc.Width * k_DepthTexelLayout.TexelBytes };

        ComPtr<ID3D11Device> device{};
        staging->GetDevice(device.GetAddressOf());

        ComPtr<ID3D11DeviceContext> context{};
        if (device != nullptr) device->GetImmediateContext(context.GetAddressOf());
        if (context == nullptr) return info;

        // One staging only: the before texture is copied to the CPU, then the after one. The after of the last pair stays for the between.
        std::vector<std::uint8_t> before{};
        std::vector<std::uint8_t> after{};
        std::vector<std::uint8_t> previousAfter{};
        bool hasPreviousAfter{ false };

        for (std::uint32_t k{ 0 }; k < info.Count; ++k)
        {
            DepthPairReport& report{ info.Pairs[k] };
            report.Note = pairs.GetNote(k);

            if (report.Note.State != PairState::Copied)
            {
                hasPreviousAfter = false;
                continue;
            }

            HRESULT result{ CopyTextureToCpu(context.Get(), staging, pairs.GetCopy(k * 2ull), desc.Width, desc.Height,
                k_DepthTexelLayout.TexelBytes, before) };

            if (SUCCEEDED(result) && hasPreviousAfter)
            {
                const std::optional<TexelDiff> between{ RenderService::CompareDepth(previousAfter.data(), pitch, before.data(), pitch, desc.Width,
                    desc.Height) };

                if (between)
                {
                    report.HasBetween = true;
                    report.Between = *between;
                }
            }

            hasPreviousAfter = false;

            if (SUCCEEDED(result))
            {
                result = CopyTextureToCpu(context.Get(), staging, pairs.GetCopy(k * 2ull + 1), desc.Width, desc.Height,
                    k_DepthTexelLayout.TexelBytes, after);
            }

            if (FAILED(result))
            {
                report.Status = StencilReadStatus::MapFailed;
                report.Result = static_cast<std::int32_t>(result);
                continue;
            }

            // The pairs of the passes 0xB and 0 keep which texels changed: the color pairs count their texels outside of the union.
            const bool isMaskPair{ report.Note.Pass == k_FirstPersonMaskPass || report.Note.Pass == k_FirstPersonZeroPass };
            ChangedMap pairMap{};
            const std::optional<TexelDiff> changed{ RenderService::CompareTexels(before.data(), pitch, after.data(), pitch, desc.Width, desc.Height,
                k_DepthTexelLayout, isMaskPair ? &pairMap : nullptr) };

            if (changed && isMaskPair) UnionChangedMap(mask, pairMap);

            if (!changed)
            {
                report.Status = StencilReadStatus::BadLayout;
                continue;
            }

            report.Status = StencilReadStatus::Read;
            report.Changed = *changed;
            previousAfter.swap(after);
            hasPreviousAfter = true;
        }

        return info;
    }

    auto RenderService::ReadColorPairs(const ChangedMap& depthMask) -> ColorProbeInfo
    {
        const CopyPairs<ColorPairNote, k_ColorPairCount>& pairs{ m_RenderStore.GetColorPairs() };
        ColorProbeInfo info{ .Count = (std::min)(pairs.GetNoteCount(), static_cast<std::uint32_t>(k_ColorPairCount)) };

        ID3D11Texture2D* staging{ pairs.GetStaging() };
        if (staging == nullptr) return info;

        const D3D11_TEXTURE2D_DESC desc{ pairs.GetDesc() };
        const std::uint32_t pitch{ desc.Width * k_ColorTexelLayout.TexelBytes };
        info.Width = desc.Width;
        info.Height = desc.Height;
        info.Format = static_cast<std::uint32_t>(desc.Format);

        ComPtr<ID3D11Device> device{};
        staging->GetDevice(device.GetAddressOf());

        ComPtr<ID3D11DeviceContext> context{};
        if (device != nullptr) device->GetImmediateContext(context.GetAddressOf());
        if (context == nullptr) return info;

        std::vector<std::uint8_t> before{};
        std::vector<std::uint8_t> after{};
        ChangedMap changedMap{};

        for (std::uint32_t k{ 0 }; k < info.Count; ++k)
        {
            ColorPairReport& report{ info.Pairs[k] };
            report.Note = pairs.GetNote(k);
            if (report.Note.State != PairState::Copied) continue;

            HRESULT result{ CopyTextureToCpu(context.Get(), staging, pairs.GetCopy(k * 2ull), desc.Width, desc.Height,
                k_ColorTexelLayout.TexelBytes, before) };

            if (SUCCEEDED(result))
            {
                result = CopyTextureToCpu(context.Get(), staging, pairs.GetCopy(k * 2ull + 1), desc.Width, desc.Height,
                    k_ColorTexelLayout.TexelBytes, after);
            }

            if (FAILED(result))
            {
                report.Status = StencilReadStatus::MapFailed;
                report.Result = static_cast<std::int32_t>(result);
                continue;
            }

            const std::optional<TexelDiff> changed{ RenderService::CompareTexels(before.data(), pitch, after.data(), pitch, desc.Width, desc.Height,
                k_ColorTexelLayout, &changedMap) };

            if (!changed)
            {
                report.Status = StencilReadStatus::BadLayout;
                continue;
            }

            report.Status = StencilReadStatus::Read;
            report.Changed = *changed;
            report.Outside = RenderService::CountOutsideMask(depthMask, changedMap);
        }

        return info;
    }

    auto RenderService::ReportDepthPairs(ChangedMap& mask) -> void
    {
        const CopyPairs<DepthPairNote, k_DepthPairCount>& pairs{ m_RenderStore.GetDepthPairs() };
        const CopyPairSetup setup{ pairs.GetSetup() };

        if (!pairs.AreReady())
        {
            if (!setup.IsAttempted)
            {
                m_Log.Message("[FirstPersonProbe] INFO: depth pairs not made (no depth stencil captured from the model draw).");
            }
            else
            {
                m_Log.Message("[FirstPersonProbe] INFO: depth pairs not made: {} hresult {:#010X}.", StencilStatusName(setup.Status),
                    static_cast<std::uint32_t>(setup.Result));
            }

            m_RenderStore.GetDepthPairs().Release();
            return;
        }

        const DepthProbeInfo info{ this->ReadDepthPairs(mask) };

        if (info.Count == 0) m_Log.Message("[FirstPersonProbe] INFO: depth pairs made but no call of the model was seen.");

        for (std::uint32_t k{ 0 }; k < info.Count; ++k)
        {
            const DepthPairReport& report{ info.Pairs[k] };
            const char* source{ report.Note.IsCaptured ? " (no depth stencil bound at the entry, captured resource)" : "" };

            if (report.Note.State == PairState::Mismatch)
            {
                m_Log.Message("[FirstPersonProbe] INFO: depth call {} caller {} not copied (the depth stencil is not the texture of the "
                    "copies){}.", k, DepthCallerName(report.Note), source);
            }
            else if (report.Status != StencilReadStatus::Read)
            {
                m_Log.Message("[FirstPersonProbe] INFO: depth call {} caller {} not read: {} hresult {:#010X}{}.", k, DepthCallerName(report.Note),
                    StencilStatusName(report.Status), static_cast<std::uint32_t>(report.Result), source);
            }
            else
            {
                const TexelDiff& changed{ report.Changed };

                m_Log.Message("[FirstPersonProbe] INFO: depth call {} caller {} changed {} box {},{}..{},{} depth {}..{} weapon {} right {} "
                    "left {}{}.", k, DepthCallerName(report.Note), changed.Changed, changed.X0, changed.Y0, changed.X1, changed.Y1, changed.MinDepth,
                    changed.MaxDepth, changed.Weapon, changed.Right, changed.Changed - changed.Right, source);
            }

            if (report.HasBetween)
            {
                m_Log.Message("[FirstPersonProbe] INFO: depth between {} and {} changed {}.", k - 1, k, report.Between.Changed);
            }
        }

        m_RenderStore.GetDepthPairs().Release();
    }

    auto RenderService::ReportColorPairs(const ChangedMap& depthMask) -> void
    {
        const CopyPairs<ColorPairNote, k_ColorPairCount>& pairs{ m_RenderStore.GetColorPairs() };
        const CopyPairSetup setup{ pairs.GetSetup() };

        if (!pairs.AreReady())
        {
            if (!setup.IsAttempted)
            {
                m_Log.Message("[FirstPersonProbe] INFO: color pairs: none (no render target 0 seen at a call of the forward pass 0x26EA78).");
            }
            else
            {
                m_Log.Message("[FirstPersonProbe] INFO: color pairs: none ({} hresult {:#010X}).", StencilStatusName(setup.Status),
                    static_cast<std::uint32_t>(setup.Result));
            }

            m_RenderStore.ReleaseColorPairs();
            return;
        }

        const ColorProbeInfo info{ this->ReadColorPairs(depthMask) };

        if (info.Count == 0) m_Log.Message("[FirstPersonProbe] INFO: color pairs: none (made but no call of the forward pass was seen).");

        const std::string target{ std::format("{} {}x{}", FormatText(info.Format), info.Width, info.Height) };
        const char* noMask{ depthMask.Texels.empty() ? "n/a (no depth pair 0xB or 0 read)" : "n/a (size differs)" };

        for (std::uint32_t k{ 0 }; k < info.Count; ++k)
        {
            const ColorPairReport& report{ info.Pairs[k] };
            const unsigned phase{ report.Note.Phase };

            if (report.Note.State == PairState::NoTarget)
            {
                m_Log.Message("[FirstPersonProbe] INFO: color call {} caller 26EA78 phase {} not copied (no render target 0 bound).", k, phase);
            }
            else if (report.Note.State == PairState::Mismatch)
            {
                m_Log.Message("[FirstPersonProbe] INFO: color call {} caller 26EA78 phase {} not copied (the render target 0 is not the texture "
                    "of the copies).", k, phase);
            }
            else if (report.Status != StencilReadStatus::Read)
            {
                m_Log.Message("[FirstPersonProbe] INFO: color call {} caller 26EA78 phase {} not read: {} hresult {:#010X}.", k, phase,
                    StencilStatusName(report.Status), static_cast<std::uint32_t>(report.Result));
            }
            else
            {
                const TexelDiff& changed{ report.Changed };
                const std::string outside{ report.Outside ? std::to_string(*report.Outside) : std::string{ noMask } };

                m_Log.Message("[FirstPersonProbe] INFO: color call {} caller 26EA78 phase {} copied changed {} box {},{}..{},{} weapon {} right {} "
                    "left {} outside depth mask {} ({}).", k, phase, changed.Changed, changed.X0, changed.Y0, changed.X1, changed.Y1, changed.Weapon,
                    changed.Right, changed.Changed - changed.Right, outside, target);
            }
        }

        m_RenderStore.ReleaseColorPairs();
    }

    auto RenderService::ReportCopyPairs() -> void
    {
        ChangedMap depthMask{};

        this->ReportDepthPairs(depthMask);
        this->ReportColorPairs(depthMask);
    }

    auto RenderService::ReduceStencil(const std::uint8_t* data, std::uint32_t rowPitch, std::uint32_t width, std::uint32_t height)
        -> StencilProbeInfo
    {
        StencilProbeInfo info{ .IsAttempted = true, .Width = width, .Height = height };

        if (data == nullptr || width == 0 || height == 0 || rowPitch < width * k_StencilTexelBytes)
        {
            info.Status = StencilReadStatus::BadLayout;
            return info;
        }

        info.Status = StencilReadStatus::Read;

        for (std::uint32_t y{ 0 }; y < height; ++y)
        {
            const std::uint8_t* row{ data + static_cast<std::size_t>(y) * rowPitch };

            for (std::uint32_t x{ 0 }; x < width; ++x)
            {
                const std::uint8_t* texel{ row + static_cast<std::size_t>(x) * k_StencilTexelBytes };

                float depth{ 0.0f };
                std::memcpy(&depth, texel, sizeof(depth));
                const std::uint8_t stencil{ texel[k_StencilByteOffset] };

                for (std::size_t i{ 0 }; i < k_StencilRegionCount; ++i)
                {
                    if (IsInStencilRegion(static_cast<StencilRegion>(i), x, y, width, height)) AddStencilTexel(info.Regions[i], depth, stencil);
                }
            }
        }

        return info;
    }

    auto RenderService::DescribeStencilRegion(StencilRegion region, const StencilRegionStats& stats) -> std::string
    {
        std::array<std::uint32_t, k_StencilValueCount> order{};
        std::iota(order.begin(), order.end(), 0u);

        std::stable_sort(order.begin(), order.end(), [&stats](std::uint32_t a, std::uint32_t b)
        {
            return stats.Histogram[a] > stats.Histogram[b];
        });

        std::string top{};

        for (std::size_t i{ 0 }; i < k_StencilTopCount; ++i)
        {
            if (stats.Histogram[order[i]] == 0) break;

            top += std::format(" {:#04X}:{}", order[i], stats.Histogram[order[i]]);
        }

        std::string bits{};

        for (std::size_t bit{ 0 }; bit < k_StencilBitCount; ++bit)
        {
            bits += std::format(" b{}:{}", bit, stats.BitCounts[bit]);
        }

        return std::format("stencil {} n={} top{} bits{} depth {}..{} zero={}", StencilRegionName(region), stats.Texels, top, bits,
            stats.MinDepth, stats.MaxDepth, stats.ZeroDepthTexels);
    }

    auto RenderService::ReadFirstPersonStencil() -> void
    {
        if (!m_RenderStore.IsFirstPersonProbeArmed() || m_RenderStore.IsStencilProbeAttempted()) return;
        if (m_RenderStore.IsResizing() || !m_RenderStore.IsReady()) return;

        ID3D11Resource* resource{ m_RenderStore.GetFirstPersonDepth() };
        if (resource == nullptr) return;

        StencilProbeInfo info{ .IsAttempted = true };

        ComPtr<ID3D11Texture2D> texture{};

        if (FAILED(resource->QueryInterface(IID_PPV_ARGS(texture.GetAddressOf()))))
        {
            info.Status = StencilReadStatus::NotTexture;
            m_RenderStore.SetStencilProbe(info);
            return;
        }

        D3D11_TEXTURE2D_DESC desc{};
        texture->GetDesc(&desc);

        info.Width = desc.Width;
        info.Height = desc.Height;
        info.ResourceFormat = static_cast<std::uint32_t>(desc.Format);

        if (desc.Format != DXGI_FORMAT_R32G8X24_TYPELESS && desc.Format != DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
        {
            info.Status = StencilReadStatus::UnsupportedFormat;
            m_RenderStore.SetStencilProbe(info);
            return;
        }

        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.BindFlags = 0;
        desc.MiscFlags = 0;

        ComPtr<ID3D11Device> device{};
        resource->GetDevice(device.GetAddressOf());

        ComPtr<ID3D11DeviceContext> context{};
        if (device != nullptr) device->GetImmediateContext(context.GetAddressOf());

        ComPtr<ID3D11Texture2D> staging{};
        const HRESULT createResult{ device != nullptr ? device->CreateTexture2D(&desc, nullptr, staging.GetAddressOf()) : E_POINTER };

        if (FAILED(createResult) || staging == nullptr || context == nullptr)
        {
            info.Status = StencilReadStatus::CreateFailed;
            info.Result = static_cast<std::int32_t>(createResult);
            m_RenderStore.SetStencilProbe(info);
            return;
        }

        context->CopyResource(staging.Get(), resource);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        const HRESULT mapResult{ context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped) };

        if (FAILED(mapResult))
        {
            info.Status = StencilReadStatus::MapFailed;
            info.Result = static_cast<std::int32_t>(mapResult);
            m_RenderStore.SetStencilProbe(info);
            return;
        }

        info = RenderService::ReduceStencil(static_cast<const std::uint8_t*>(mapped.pData), mapped.RowPitch, desc.Width, desc.Height);
        context->Unmap(staging.Get(), 0);

        info.ResourceFormat = static_cast<std::uint32_t>(desc.Format);
        m_RenderStore.SetStencilProbe(info);
    }

    auto RenderService::ReportFirstPersonProbe() -> void
    {
        for (std::size_t i{ 0 }; i < k_FirstPersonCallerCount; ++i)
        {
            const FirstPersonSample sample{ m_RenderStore.GetFirstPersonSample(static_cast<FirstPersonCaller>(i)) };
            if (sample.CallsPerFrame == 0) continue;

            m_Log.Message("[FirstPersonProbe] INFO: {}.", DescribeFirstPerson(sample));
        }

        m_Log.Message("[FirstPersonProbe] INFO: {}.",
            DescribeChud(m_RenderStore.GetHudHook(), m_RenderStore.GetFirstPersonSample(FirstPersonCaller::ViewPhase)));

        const StencilProbeInfo stencil{ m_RenderStore.GetStencilProbe() };
        const std::string state{ std::format("hide first person {} drive {}", m_RenderStore.IsFirstPersonHidden() ? "on" : "off",
            m_RenderStore.IsFirstPersonDriveActive() ? "on" : "off") };

        if (!stencil.IsAttempted)
        {
            m_Log.Message("[FirstPersonProbe] INFO: stencil not read (no depth stencil captured from the model draw or no HUD draw on the thread "
                "of Present) | {}.", state);
            this->ReportCopyPairs();
            return;
        }

        if (stencil.Status != StencilReadStatus::Read)
        {
            m_Log.Message("[FirstPersonProbe] INFO: stencil not read: {} hresult {:#010X} texture {}x{} res {} | {}.",
                StencilStatusName(stencil.Status), static_cast<std::uint32_t>(stencil.Result), stencil.Width, stencil.Height,
                FormatText(stencil.ResourceFormat), state);
            this->ReportCopyPairs();
            return;
        }

        for (std::size_t i{ 0 }; i < k_StencilRegionCount; ++i)
        {
            m_Log.Message("[FirstPersonProbe] INFO: {} | {}.", RenderService::DescribeStencilRegion(static_cast<StencilRegion>(i), stencil.Regions[i]),
                state);
        }

        this->ReportCopyPairs();
    }

    auto RenderService::WaitForIdle(milliseconds timeout) -> bool
    {
        const auto deadline = steady_clock::now() + timeout;

        while (m_RenderStore.GetInFlight() > 0)
        {
            if (steady_clock::now() >= deadline) return false;
            std::this_thread::sleep_for(microseconds{ 100 });
        }

        return true;
    }

    auto RenderService::Shutdown() -> void
    {
        std::lock_guard lock{ m_ShutdownMutex };

        if (m_RenderStore.IsShutdownDone()) return;

        for (auto it = m_OnShutdown.rbegin(); it != m_OnShutdown.rend(); ++it)
        {
            try
            {
                (*it)();
            }
            catch (...)
            {
                this->ReportHandlerError("OnShutdown");
            }
        }

        m_RenderStore.Cleanup();
        m_RenderStore.SetShutdownDone();

        m_Log.Message("[RenderService] INFO: Shutdown complete.");
    }
}