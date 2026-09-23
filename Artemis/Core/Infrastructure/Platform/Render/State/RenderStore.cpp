module;

#include <d3d11.h>
#include <wrl/client.h>

module Platform.Render.State;

namespace Platform::Render::State
{
	auto RenderStore::GetDevice() const -> ID3D11Device*
	{
		return m_Device.Get();
	}

	auto RenderStore::GetContext() const -> ID3D11DeviceContext*
	{
		return m_Context.Get();
	}

	auto RenderStore::GetWindow() const -> HWND
	{
		return m_Window;
	}

	auto RenderStore::GetRenderTarget() const -> ID3D11RenderTargetView*
	{
		return m_RenderTarget.Get();
	}

	auto RenderStore::GetDepthStencil() const -> ID3D11DepthStencilView*
	{
		return m_DepthStencil.Get();
	}

	auto RenderStore::GetSwapChain() const -> const void*
	{
		return m_SwapChain;
	}

	auto RenderStore::GetWidth() const -> std::uint32_t
	{
		return m_Width.load();
	}

	auto RenderStore::GetHeight() const -> std::uint32_t
	{
		return m_Height.load();
	}

	auto RenderStore::IsReady() const -> bool
	{
		return m_RenderTarget != nullptr;
	}

	auto RenderStore::SetDevice(ComPtr<ID3D11Device> device,
		ComPtr<ID3D11DeviceContext> context, HWND window,
		const void* swapChain) -> void
	{
		m_Device = std::move(device);
		m_Context = std::move(context);
		m_Window = window;
		m_SwapChain = swapChain;
	}

	auto RenderStore::SetTargets(ComPtr<ID3D11RenderTargetView> renderTarget,
		ComPtr<ID3D11DepthStencilView> depthStencil,
		std::uint32_t width, std::uint32_t height) -> void
	{
		m_RenderTarget = std::move(renderTarget);
		m_DepthStencil = std::move(depthStencil);

		m_Width.store(width);
		m_Height.store(height);
	}

	auto RenderStore::ReleaseTargets() -> void
	{
		m_RenderTarget.Reset();
		m_DepthStencil.Reset();
	}

	auto RenderStore::Cleanup() -> void
	{
		this->ReleaseTargets();

		m_Context.Reset();
		m_Device.Reset();
		m_Window = nullptr;
		m_SwapChain = nullptr;

		m_Width.store(0);
		m_Height.store(0);
	}

	auto RenderStore::IsResizing() const -> bool
	{
		return m_IsResizing.load();
	}

	auto RenderStore::SetResizing(bool value) -> void
	{
		m_IsResizing.store(value);
	}

	auto RenderStore::IsShutdownRequested() const -> bool
	{
		return m_IsShutdownRequested.load(std::memory_order_acquire);
	}

	auto RenderStore::RequestShutdown() -> void
	{
		m_IsShutdownRequested.store(true, std::memory_order_release);
	}

	auto RenderStore::IsShutdownDone() const -> bool
	{
		return m_IsShutdownDone.load(std::memory_order_acquire);
	}

	auto RenderStore::SetShutdownDone() -> void
	{
		m_IsShutdownDone.store(true, std::memory_order_release);
	}

	auto RenderStore::GetInFlight() const -> int
	{
		return m_InFlight.load(std::memory_order_acquire);
	}
}