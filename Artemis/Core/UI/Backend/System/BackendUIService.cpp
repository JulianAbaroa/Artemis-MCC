module;

#include <windows.h>
#include <d3d11.h>
#include "External/imgui/imgui.h"
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

module UI.Backend.System;
import :ImGui;

import std;

extern "C++"
{
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}

namespace
{
	constexpr float k_MinimumAlpha = 0.20f;
	constexpr float k_MinimumScale = 1.0f;
	constexpr float k_MaximumScale = 4.0f;

	auto ApplyDefaultStyle() -> void
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

	auto LoadFont(std::uint32_t width) -> void
	{
		float fontSize = 22.0f;
		if (width >= 3840) fontSize = 38.0f;
		else if (width >= 2560) fontSize = 30.0f;

		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", fontSize);
		if (font == nullptr) font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", fontSize);
		if (font == nullptr) io.Fonts->AddFontDefault();
	}

	auto IsMouseMessage(std::uint32_t message) -> bool
	{
		return message >= WM_MOUSEFIRST && message <= WM_MOUSELAST;
	}

	auto IsKeyboardMessage(std::uint32_t message) -> bool
	{
		return message >= WM_KEYFIRST && message <= WM_KEYLAST;
	}
}

namespace UI::Backend::System
{
	auto BackendUIService::Initialize(const FrameContext& frame) -> void
	{
		if (frame.Device == nullptr || frame.Context == nullptr
			|| frame.Window == nullptr)
		{
			m_LogsService.Message("[BackendUIService] ERROR:"
				" Initialize called with an incomplete FrameContext.");
			return;
		}

		if (m_IsInitialized.load()) this->Shutdown();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		LoadFont(frame.Width);
		ApplyDefaultStyle();
		m_AppliedScale = 0.0f;

		if (!ImGui_ImplWin32_Init(frame.Window))
		{
			m_LogsService.Message("[BackendUIService] ERROR:"
				" ImGui_ImplWin32_Init failed.");
			ImGui::DestroyContext();
			return;
		}

		if (!ImGui_ImplDX11_Init(frame.Device, frame.Context))
		{
			m_LogsService.Message("[BackendUIService] ERROR:"
				" ImGui_ImplDX11_Init failed.");
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			return;
		}

		m_IsInitialized.store(true);
		m_LogsService.Message("[BackendUIService] INFO:"
			" Backend initialized ({}x{}).", frame.Width, frame.Height);
	}

	auto BackendUIService::OnResize(const FrameContext& frame) -> void
	{
		if (!m_IsInitialized.load())
		{
			this->Initialize(frame);
			return;
		}

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();

		m_LogsService.Message("[BackendUIService] INFO:"
			" Refreshed device objects ({}x{}).", frame.Width, frame.Height);
	}

	auto BackendUIService::Shutdown() -> void
	{
		if (!m_IsInitialized.exchange(false)) return;

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		m_LogsService.Message("[BackendUIService] INFO: Backend shut down.");
	}

	auto BackendUIService::NewFrame(bool isMenuVisible) -> void
	{
		if (!m_IsInitialized.load()) return;

		this->ApplyPreferences();
		this->ApplyInputMode(isMenuVisible);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	auto BackendUIService::Render() -> void
	{
		if (!m_IsInitialized.load()) return;

		ImGui::Render();

		auto* context = m_RenderStore.GetContext();
		auto* renderTarget = m_RenderStore.GetRenderTarget();
		if (context == nullptr || renderTarget == nullptr) return;

		ID3D11RenderTargetView* previousTarget = nullptr;
		ID3D11DepthStencilView* previousDepth = nullptr;
		context->OMGetRenderTargets(1, &previousTarget, &previousDepth);

		context->OMSetRenderTargets(1, &renderTarget, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		context->OMSetRenderTargets(1, &previousTarget, previousDepth);

		if (previousTarget) previousTarget->Release();
		if (previousDepth) previousDepth->Release();
	}

	auto BackendUIService::ForwardMessage(WindowMessage& message) -> bool
	{
		if (!m_IsInitialized.load() || ImGui::GetCurrentContext() == nullptr) return false;

		const LRESULT result = ImGui_ImplWin32_WndProcHandler(
			static_cast<HWND>(message.Window), message.Message,
			static_cast<WPARAM>(message.WParam), static_cast<LPARAM>(message.LParam));

		message.Result = static_cast<std::intptr_t>(result);

		return result != 0;
	}

	auto BackendUIService::WantsCapture(std::uint32_t message) const -> bool
	{
		if (!m_IsInitialized.load() || ImGui::GetCurrentContext() == nullptr) return false;

		const ImGuiIO& io = ImGui::GetIO();

		if (IsMouseMessage(message)) return io.WantCaptureMouse;
		if (IsKeyboardMessage(message)) return io.WantCaptureKeyboard || io.WantTextInput;

		return false;
	}

	auto BackendUIService::IsInitialized() const -> bool
	{
		return m_IsInitialized.load();
	}

	auto BackendUIService::IsReady() const -> bool
	{
		return m_IsInitialized.load() && m_RenderStore.IsReady();
	}

	auto BackendUIService::ApplyPreferences() -> void
	{
		const float scale = std::clamp(m_SettingsStore.GetUIScale(),
			k_MinimumScale, k_MaximumScale);

		if (scale != m_AppliedScale)
		{
			ApplyDefaultStyle();
			ImGui::GetStyle().ScaleAllSizes(scale);

#if IMGUI_VERSION_NUM >= 19200
			ImGui::GetStyle().FontScaleMain = scale;
#else
			ImGui::GetIO().FontGlobalScale = scale;
#endif

			m_AppliedScale = scale;
		}

		ImGui::GetStyle().Alpha = (std::max)(m_SettingsStore.GetMenuAlpha(), k_MinimumAlpha);
	}

	auto BackendUIService::ApplyInputMode(bool isMenuVisible) -> void
	{
		ImGuiIO& io = ImGui::GetIO();

		io.MouseDrawCursor = isMenuVisible;

		if (isMenuVisible)
		{
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
		}
		else
		{
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
			io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		}
	}
}