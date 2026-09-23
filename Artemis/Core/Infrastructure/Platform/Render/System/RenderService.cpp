module;

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

module Platform.Render.System;
import :RenderService;

import std;

namespace Platform::Render::System
{
	auto RenderService::OnInitialized(FrameHandler handler) -> void
	{
		m_OnInitialized.push_back(std::move(handler));
	}

	auto RenderService::OnFrame(FrameHandler handler) -> void
	{
		m_OnFrame.push_back(std::move(handler));
	}

	auto RenderService::OnResize(FrameHandler handler) -> void
	{
		m_OnResize.push_back(std::move(handler));
	}

	auto RenderService::OnShutdown(Callback callback) -> void
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

		const bool isAnotherSwapChain = m_RenderStore.GetSwapChain() != swapChain;
		if (!m_RenderStore.IsReady() || isAnotherSwapChain)
		{
			this->Bind(swapChain);
			if (!m_RenderStore.IsReady()) return;
		}

		this->Raise(m_OnFrame, "OnFrame");
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
		auto fail = [this](const char* reason) {
			if (!m_HasAttachFailed)
			{
				m_LogsService.Message("[RenderService] ERROR: {}", reason);
				m_HasAttachFailed = true;
			}
			return false;
			};

		ComPtr<ID3D11Device> device;
		if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(device.GetAddressOf()))))
		{
			return fail("The swap chain does not belong to a D3D11 device.");
		}

		ComPtr<ID3D11DeviceContext> context;
		device->GetImmediateContext(context.GetAddressOf());

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		if (FAILED(swapChain->GetDesc(&swapChainDesc)))
		{
			return fail("IDXGISwapChain::GetDesc failed.");
		}

		ComPtr<ID3D11Texture2D> backBuffer;
		if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()))))
		{
			return fail("IDXGISwapChain::GetBuffer failed.");
		}

		D3D11_TEXTURE2D_DESC bufferDesc = {};
		backBuffer->GetDesc(&bufferDesc);

		ComPtr<ID3D11RenderTargetView> renderTarget;
		if (FAILED(device->CreateRenderTargetView(backBuffer.Get(), nullptr,
			renderTarget.GetAddressOf())))
		{
			return fail("CreateRenderTargetView failed.");
		}

		D3D11_TEXTURE2D_DESC depthDesc = {};
		depthDesc.Width = bufferDesc.Width;
		depthDesc.Height = bufferDesc.Height;
		depthDesc.MipLevels = 1;
		depthDesc.ArraySize = 1;
		depthDesc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
		depthDesc.SampleDesc = bufferDesc.SampleDesc;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		ComPtr<ID3D11DepthStencilView> depthStencil;
		ComPtr<ID3D11Texture2D> depthTexture;

		if (SUCCEEDED(device->CreateTexture2D(&depthDesc, nullptr,
			depthTexture.GetAddressOf())))
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
			viewDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			viewDesc.ViewDimension = bufferDesc.SampleDesc.Count > 1 ?
				D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;

			if (FAILED(device->CreateDepthStencilView(depthTexture.Get(),
				&viewDesc, depthStencil.GetAddressOf())))
			{
				m_LogsService.Message("[RenderService] ERROR:"
					" CreateDepthStencilView failed.");
			}
		}
		else
		{
			m_LogsService.Message("[RenderService] ERROR:"
				" Depth CreateTexture2D failed.");
		}

		m_RenderStore.SetDevice(std::move(device), std::move(context),
			swapChainDesc.OutputWindow, swapChain);

		m_RenderStore.SetTargets(std::move(renderTarget), std::move(depthStencil),
			bufferDesc.Width, bufferDesc.Height);

		m_HasAttachFailed = false;
		return true;
	}

	auto RenderService::MakeContext() const -> FrameContext
	{
		FrameContext frame{};
		frame.Device = m_RenderStore.GetDevice();
		frame.Context = m_RenderStore.GetContext();
		frame.RenderTarget = m_RenderStore.GetRenderTarget();
		frame.DepthStencil = m_RenderStore.GetDepthStencil();
		frame.Window = m_RenderStore.GetWindow();
		frame.Width = m_RenderStore.GetWidth();
		frame.Height = m_RenderStore.GetHeight();
		return frame;
	}

	auto RenderService::Raise(const std::vector<FrameHandler>& handlers,
		const char* stage) -> void
	{
		const FrameContext frame = this->MakeContext();

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

	auto RenderService::ReportHandlerError(const char* stage) -> void
	{
		if (m_HandlerErrors.fetch_add(1) >= 5) return;

		m_LogsService.Message("[RenderService] ERROR: A {} handler threw an exception.", stage);
	}

	auto RenderService::RequestShutdown() -> void
	{
		m_RenderStore.RequestShutdown();
	}

	auto RenderService::WaitForShutdown(MilliSeconds timeout) -> bool
	{
		const auto deadline = SteadyClock::now() + timeout;

		while (!m_RenderStore.IsShutdownDone())
		{
			if (SteadyClock::now() >= deadline) return false;
			std::this_thread::sleep_for(MilliSeconds(1));
		}

		return true;
	}

	auto RenderService::WaitForIdle(MilliSeconds timeout) -> bool
	{
		const auto deadline = SteadyClock::now() + timeout;

		while (m_RenderStore.GetInFlight() > 0)
		{
			if (SteadyClock::now() >= deadline) return false;
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

		return true;
	}

	auto RenderService::Shutdown() -> void
	{
		std::lock_guard<std::mutex> lock(m_ShutdownMutex);

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

		m_LogsService.Message("[RenderService] INFO: Shutdown complete.");
	}
}