module;

#include <d3d11.h>

export module Platform.Render.Type;

import std;

export namespace Platform::Render::Type
{
	struct Vertex
	{
		float X{}, Y{}, Z{};
		float R{}, G{}, B{};
	};

	struct SwapChainAddresses
	{
		void* Present{ nullptr };
		void* ResizeBuffers{ nullptr };
	};

	struct FrameContext
	{
		ID3D11Device* Device{ nullptr };
		ID3D11DeviceContext* Context{ nullptr };
		ID3D11RenderTargetView* RenderTarget{ nullptr };
		ID3D11DepthStencilView* DepthStencil{ nullptr };
		HWND Window{ nullptr };

		std::uint32_t Width{ 0 };
		std::uint32_t Height{ 0 };
	};
}