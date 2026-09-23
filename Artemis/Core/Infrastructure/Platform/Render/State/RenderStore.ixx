module;

#include <d3d11.h>
#include <wrl/client.h>

export module Platform.Render.State;

import std;

export namespace Platform::Render::State
{
	class RenderStore
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

	public:
		RenderStore() = default;
		~RenderStore() = default;

		RenderStore(const RenderStore&) = delete;
		RenderStore& operator=(const RenderStore&) = delete;

		class HookScope
		{
		public:
			explicit HookScope(RenderStore& store) : m_Store(store)
			{
				m_Store.m_InFlight.fetch_add(1, std::memory_order_acq_rel);
			}
			~HookScope()
			{
				m_Store.m_InFlight.fetch_sub(1, std::memory_order_acq_rel);
			}

			HookScope(const HookScope&) = delete;
			HookScope& operator=(const HookScope&) = delete;

		private:
			RenderStore& m_Store;
		};

		auto GetDevice() const -> ID3D11Device*;
		auto GetContext() const -> ID3D11DeviceContext*;
		auto GetWindow() const -> HWND;
		auto GetRenderTarget() const -> ID3D11RenderTargetView*;
		auto GetDepthStencil() const -> ID3D11DepthStencilView*;

		auto GetSwapChain() const -> const void*;

		auto GetWidth() const -> std::uint32_t;
		auto GetHeight() const -> std::uint32_t;

		auto IsReady() const -> bool;

		auto SetDevice(ComPtr<ID3D11Device> device,
			ComPtr<ID3D11DeviceContext> context, HWND window,
			const void* swapChain) -> void;

		auto SetTargets(ComPtr<ID3D11RenderTargetView> renderTarget,
			ComPtr<ID3D11DepthStencilView> depthStencil,
			std::uint32_t width, std::uint32_t height) -> void;

		auto ReleaseTargets() -> void;

		auto Cleanup() -> void;

		auto IsResizing() const -> bool;
		auto SetResizing(bool value) -> void;

		auto IsShutdownRequested() const -> bool;
		auto RequestShutdown() -> void;

		auto IsShutdownDone() const -> bool;
		auto SetShutdownDone() -> void;

		auto GetInFlight() const -> int;

	private:
		ComPtr<ID3D11Device> m_Device{};
		ComPtr<ID3D11DeviceContext> m_Context{};
		ComPtr<ID3D11RenderTargetView> m_RenderTarget{};
		ComPtr<ID3D11DepthStencilView> m_DepthStencil{};
		HWND m_Window{ nullptr };
		const void* m_SwapChain{ nullptr };

		std::atomic<std::uint32_t> m_Width{ 0 };
		std::atomic<std::uint32_t> m_Height{ 0 };

		std::atomic<bool> m_IsResizing{ false };
		std::atomic<bool> m_IsShutdownRequested{ false };
		std::atomic<bool> m_IsShutdownDone{ false };
		std::atomic<int> m_InFlight{ 0 };
	};
}