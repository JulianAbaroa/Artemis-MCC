module;

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")

module Platform.Render.System;
import :SwapChainLocator;

import std;

namespace
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	struct DummyWindow
	{
		static constexpr const wchar_t* k_ClassName = L"ArtemisDummyWindow";

		HINSTANCE Instance{ GetModuleHandleW(nullptr) };
		HWND Handle{ nullptr };
		bool IsClassRegistered{ false };

		DummyWindow() = default;
		~DummyWindow()
		{
			if (Handle) DestroyWindow(Handle);
			if (IsClassRegistered) UnregisterClassW(k_ClassName, Instance);
		}

		DummyWindow(const DummyWindow&) = delete;
		DummyWindow& operator=(const DummyWindow&) = delete;

		auto Create() -> bool
		{
			WNDCLASSEXW windowClass = {};
			windowClass.cbSize = sizeof(windowClass);
			windowClass.style = CS_HREDRAW | CS_VREDRAW;
			windowClass.lpfnWndProc = DefWindowProcW;
			windowClass.hInstance = Instance;
			windowClass.lpszClassName = k_ClassName;

			if (!RegisterClassExW(&windowClass)) return false;
			IsClassRegistered = true;

			Handle = CreateWindowExW(0, k_ClassName, L"Artemis Dummy Window",
				WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr,
				Instance, nullptr);

			return Handle != nullptr;
		}
	};
}

namespace Platform::Render::System
{
	auto SwapChainLocator::Locate() -> SwapChainAddresses
	{
		if (m_Addresses.Present && m_Addresses.ResizeBuffers) return m_Addresses;

		DummyWindow window;
		if (!window.Create())
		{
			m_LogsService.Message("[SwapChainLocator] ERROR:"
				" Failed to create the dummy window.");
			return {};
		}

		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferCount = 1;
		desc.BufferDesc.Width = 800;
		desc.BufferDesc.Height = 600;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.OutputWindow = window.Handle;
		desc.SampleDesc.Count = 1;
		desc.Windowed = TRUE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		ComPtr<IDXGISwapChain> swapChain;
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;

		const HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr,
			D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
			&desc, swapChain.GetAddressOf(), device.GetAddressOf(), nullptr,
			context.GetAddressOf());

		if (FAILED(result))
		{
			m_LogsService.Message("[SwapChainLocator] ERROR:"
				" D3D11CreateDeviceAndSwapChain failed (0x{:08X}).",
				static_cast<std::uint32_t>(result));
			return {};
		}

		void** vtable = *reinterpret_cast<void***>(swapChain.Get());

		m_Addresses.Present = vtable[k_PresentIndex];
		m_Addresses.ResizeBuffers = vtable[k_ResizeBuffersIndex];

		return m_Addresses;
	}
}