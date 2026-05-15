#include "pch.h"

// Header.
#include "System_Render.h"

// --- Hooks ---

#include "Core/Hooks/Infrastructure/Window/Hook_WndProc.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Render/State_Render.h"

#include "Core/States/Infrastructure/Persistence/State_Settings.h"

// --- Systems ---

#include "Core/Systems/Interface/Debug/System_Debug.h"

#include "External/imgui/backends/imgui_impl_win32.h"
#include "External/imgui/backends/imgui_impl_dx11.h"

DX11Addresses System_Render::GetVtableAddresses() const
{
    DX11Addresses addr = { nullptr, nullptr };

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"DummyWindowClass";

    if (!RegisterClassEx(&wc))
    {
        m_Deps.System_Debug.Log("[DXUtil] ERROR: RegisterClassEx failed");
        return addr;
    }

    HWND hWnd = CreateWindowEx(
        0, wc.lpszClassName, L"Dummy Window",
        WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
        NULL, NULL, wc.hInstance, NULL);

    if (!hWnd)
    {
        m_Deps.System_Debug.Log("[DXUtil] ERROR: CreateWindowEx failed");
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return addr;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* pDevice = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11DeviceContext* pContext = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        0, NULL, 0, D3D11_SDK_VERSION, &sd,
        &pSwapChain, &pDevice, NULL, &pContext);

    if (SUCCEEDED(hr))
    {
        void** pVMT = *reinterpret_cast<void***>(pSwapChain);
        addr.Present = pVMT[m_PresentVMTIndex];
        addr.ResizeBuffers = pVMT[m_ResizeBuffersVMTIndex];
    }
    else
    {
        m_Deps.System_Debug.Log("[DXUtil] ERROR: D3D11CreateDeviceAndSwapChain failed");
    }

    if (pContext) pContext->Release();
    if (pDevice) pDevice->Release();
    if (pSwapChain) pSwapChain->Release();

    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return addr;
}

void System_Render::Initialize(IDXGISwapChain* pSwapChain)
{
    ID3D11Device* device = nullptr;
    if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device))) return;

    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);

    m_Deps.State_Render.SetDevice(device);
    m_Deps.State_Render.SetContext(context);

    DXGI_SWAP_CHAIN_DESC sd;
    pSwapChain->GetDesc(&sd);
    m_Deps.State_Render.SetHWND(sd.OutputWindow);

    if (!m_Deps.State_Render.IsImGuiInitialized())
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        UINT width = sd.BufferDesc.Width;
        UINT height = sd.BufferDesc.Height;
        int evenWidth = static_cast<int>(width & ~1);
        int evenHeight = static_cast<int>(height & ~1);
        m_Deps.State_Render.SetWidth(evenWidth);
        m_Deps.State_Render.SetHeight(evenHeight);

        float baseFontSize = 22.0f;
        if (width >= 2560) baseFontSize = 30.0f;
        if (width >= 3840) baseFontSize = 38.0f;

        float savedScale = m_Deps.State_Render.GetUIScale();
        if (savedScale < 0.8f || savedScale > 4.0f) savedScale = 1.0f;

        float finalFontSize = baseFontSize * savedScale;

        ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", finalFontSize);
        if (!font) font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", finalFontSize);

        this->ApplyCustomStyle();

        if (m_Deps.State_Settings.ShouldUseAppData())
        {
            ImGui::GetStyle().Alpha = 
                m_Deps.State_Settings.GetMenuAlpha();
        }

        ImGui::GetStyle().ScaleAllSizes(savedScale);

        ImGui_ImplWin32_Init(sd.OutputWindow);
        ImGui_ImplDX11_Init(device, context);

        bool showOnStart = m_Deps.State_Settings.ShouldOpenUIOnStart();
        m_Deps.State_Settings.SetMenuVisible(showOnStart);

        m_Deps.State_Render.SetImGuiInitialized(true);
        m_Deps.System_Debug.Log("[RenderSystem] INFO: ImGui Initialized for the first time.");
    }
    else
    {
        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui_ImplDX11_CreateDeviceObjects();
        m_Deps.System_Debug.Log("[RenderSystem] INFO: Device Objects Refreshed (Resize).");
    }

    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
    {
        ID3D11RenderTargetView* rtv = nullptr;
        device->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
        m_Deps.State_Render.SetRTV(rtv);
        pBackBuffer->Release();
    }
}

bool System_Render::IsInitialized()
{
    return m_Deps.State_Render.GetRTV() != nullptr;
}

void System_Render::Shutdown()
{
    if (m_Deps.State_Render.IsImGuiInitialized() && ImGui::GetCurrentContext())
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_Deps.State_Render.SetImGuiInitialized(false);
    }

    m_Deps.State_Render.FullCleanup();
    m_Deps.System_Debug.Log("[RenderSystem] INFO: Shutdown complete.");
}


void System_Render::UpdateFramerate()
{
    m_FrameCount++;
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - m_LastFramerateTime).count() >= 1)
    {
        m_Deps.State_Render.SetFramerate(m_FrameCount.load());
        m_FrameCount = 0;
        m_LastFramerateTime = now;
    }
}

void System_Render::BeginFrame(IDXGISwapChain* pSwapChain)
{
    if (m_Deps.State_Render.GetRTV() == nullptr)
    {
        this->Initialize(pSwapChain);
        if (m_Deps.State_Render.GetRTV() == nullptr) return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool menuVisible = m_Deps.State_Settings.IsMenuVisible();
    ImGuiIO& io = ImGui::GetIO();

    if (menuVisible)
    {
        if (m_Deps.State_Settings.ShouldFreezeMouse() && 
            GetForegroundWindow() == m_Deps.State_Render.GetHWND())
        {
            RECT rect;
            GetWindowRect(m_Deps.State_Render.GetHWND(), &rect);
            ClipCursor(&rect);
        }
        
        io.MouseDrawCursor = true;
    }
    else 
    {
        if (io.MouseDrawCursor)
        {
            ClipCursor(NULL);
            io.MouseDrawCursor = false;
        }
    }
}

void System_Render::EndFrame()
{
    ImGui::Render();

    auto rtv = m_Deps.State_Render.GetRTV();
    if (rtv)
    {
        auto ctx = m_Deps.State_Render.GetContext();

        ID3D11RenderTargetView* oldRTV = nullptr;
        ID3D11DepthStencilView* oldDSV = nullptr;
        ctx->OMGetRenderTargets(1, &oldRTV, &oldDSV);

        ctx->OMSetRenderTargets(1, &rtv, NULL);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        ctx->OMSetRenderTargets(1, &oldRTV, oldDSV);

        if (oldRTV) oldRTV->Release();
        if (oldDSV) oldDSV->Release();
    }
}


void System_Render::UpdateUIScale()
{
    if (!m_Deps.State_Render.ShouldRebuildFonts()) return;

    float newScale = m_Deps.State_Render.GetUIScale();
    if (newScale < 1.0f) newScale = 1.0f;

    UINT width = m_Deps.State_Render.GetWidth();
    float baseFontSize = 22.0f;
    if (width >= 2560) baseFontSize = 30.0f;
    if (width >= 3840) baseFontSize = 38.0f;

    float finalFontSize = baseFontSize * newScale;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 oldDisplaySize = io.DisplaySize;

    io.Fonts->Clear();

    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", finalFontSize);
    if (!font) io.Fonts->AddFontDefault();

    ImGui_ImplDX11_InvalidateDeviceObjects();
    ImGui_ImplDX11_CreateDeviceObjects();

    io.DisplaySize = oldDisplaySize;

    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    this->ApplyCustomStyle();

    style.ScaleAllSizes(newScale);
    style.Alpha = m_Deps.State_Settings.GetMenuAlpha();

    m_Deps.State_Render.ResetFontRebuild();
}


void System_Render::ApplyCustomStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style = ImGuiStyle();

    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.FramePadding = ImVec2(8, 6);
    style.WindowPadding = ImVec2(12, 12);
    style.ItemSpacing = ImVec2(10, 8);
    style.ScrollbarSize = 15.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_Tab] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.2f, 0.4f, 0.6f, 1.0f);
}