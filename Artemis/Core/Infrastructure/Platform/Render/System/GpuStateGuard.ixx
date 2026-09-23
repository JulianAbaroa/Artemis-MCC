module;

#include <d3d11.h>
#include <wrl/client.h>

export module Platform.Render.System:GpuStateGuard;

import std;

export namespace Platform::Render::System
{
	class GpuStateGuard
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

	public:
		explicit GpuStateGuard(ID3D11DeviceContext* context);
		~GpuStateGuard();

		GpuStateGuard(const GpuStateGuard&) = delete;
		GpuStateGuard& operator=(const GpuStateGuard&) = delete;

	private:
		ID3D11DeviceContext* m_Context{ nullptr };

		ComPtr<ID3D11InputLayout> m_InputLayout{};
		D3D11_PRIMITIVE_TOPOLOGY m_Topology{ D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED };

		ComPtr<ID3D11Buffer> m_VertexBuffer{};
		UINT m_VertexStride{ 0 };
		UINT m_VertexOffset{ 0 };

		ComPtr<ID3D11Buffer> m_IndexBuffer{};
		DXGI_FORMAT m_IndexFormat{ DXGI_FORMAT_UNKNOWN };
		UINT m_IndexOffset{ 0 };

		ComPtr<ID3D11VertexShader> m_VertexShader{};
		ComPtr<ID3D11Buffer> m_VertexConstantBuffer{};
		ComPtr<ID3D11HullShader> m_HullShader{};
		ComPtr<ID3D11DomainShader> m_DomainShader{};
		ComPtr<ID3D11GeometryShader> m_GeometryShader{};
		ComPtr<ID3D11PixelShader> m_PixelShader{};

		ComPtr<ID3D11RasterizerState> m_Rasterizer{};
		UINT m_ViewportCount{ D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE };
		std::array<D3D11_VIEWPORT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> m_Viewports{};

		ComPtr<ID3D11RenderTargetView> m_RenderTarget{};
		ComPtr<ID3D11DepthStencilView> m_DepthStencil{};
		ComPtr<ID3D11DepthStencilState> m_DepthState{};
		UINT m_StencilRef{ 0 };
		ComPtr<ID3D11BlendState> m_BlendState{};
		std::array<float, 4> m_BlendFactor{ 0.0f, 0.0f, 0.0f, 0.0f };
		UINT m_SampleMask{ 0xffffffff };
	};
}