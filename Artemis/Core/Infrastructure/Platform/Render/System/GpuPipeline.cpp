module;

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#pragma comment(lib, "d3dcompiler.lib")

module Platform.Render.System;
import :GpuPipeline;
import :Shaders;

import std;

namespace
{
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	auto CompileShader(const char* entry, const char* target,
		Service::Logs::System::LogsService& logs, ComPtr<ID3DBlob>& blob) -> bool
	{
		UINT flags = 0;
#if defined(_DEBUG)
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		const char* source = Platform::Render::System::Shaders::k_MapSource;

		ComPtr<ID3DBlob> errors;
		const HRESULT result = D3DCompile(source, std::strlen(source),
			nullptr, nullptr, nullptr, entry, target, flags, 0,
			blob.GetAddressOf(), errors.GetAddressOf());

		if (FAILED(result))
		{
			std::string text = errors ?
				std::string(static_cast<const char*>(errors->GetBufferPointer())) :
				std::string("unknown error");

			logs.Message("[GpuPipeline] ERROR: {} shader failed to compile: {}", entry, text);
			return false;
		}

		return true;
	}
}

namespace Platform::Render::System
{
	auto GpuPipeline::Initialize(ID3D11Device* device) -> bool
	{
		if (m_IsReady) return true;

		if (!device)
		{
			m_LogsService.Message("[GpuPipeline] ERROR: No device.");
			return false;
		}

		if (!this->CompileShaders(device) || !this->CreateBuffers(device) ||
			!this->CreateStates(device))
		{
			this->Release();
			return false;
		}

		m_IsReady = true;
		m_LogsService.Message("[GpuPipeline] INFO: Ready.");
		return true;
	}

	auto GpuPipeline::IsReady() const -> bool
	{
		return m_IsReady;
	}

	auto GpuPipeline::CompileShaders(ID3D11Device* device) -> bool
	{
		ComPtr<ID3DBlob> vertexBlob;
		ComPtr<ID3DBlob> pixelBlob;

		if (!CompileShader("VSMain", "vs_5_0", m_LogsService, vertexBlob)) return false;
		if (!CompileShader("PSMain", "ps_5_0", m_LogsService, pixelBlob)) return false;

		if (FAILED(device->CreateVertexShader(vertexBlob->GetBufferPointer(),
			vertexBlob->GetBufferSize(), nullptr, m_VertexShader.GetAddressOf())))
		{
			m_LogsService.Message("[GpuPipeline] ERROR: CreateVertexShader failed.");
			return false;
		}

		if (FAILED(device->CreatePixelShader(pixelBlob->GetBufferPointer(),
			pixelBlob->GetBufferSize(), nullptr, m_PixelShader.GetAddressOf())))
		{
			m_LogsService.Message("[GpuPipeline] ERROR: CreatePixelShader failed.");
			return false;
		}

		const D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
			  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		if (FAILED(device->CreateInputLayout(layout, 2,
			vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
			m_InputLayout.GetAddressOf())))
		{
			m_LogsService.Message("[GpuPipeline] ERROR: CreateInputLayout failed.");
			return false;
		}

		return true;
	}

	auto GpuPipeline::CreateBuffers(ID3D11Device* device) -> bool
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(Matrix);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		if (FAILED(device->CreateBuffer(&desc, nullptr, m_CameraBuffer.GetAddressOf())))
		{
			m_LogsService.Message("[GpuPipeline] ERROR: Camera constant buffer failed.");
			return false;
		}

		return true;
	}

	auto GpuPipeline::CreateStates(ID3D11Device* device) -> bool
	{
		D3D11_RASTERIZER_DESC raster = {};
		raster.FillMode = D3D11_FILL_SOLID;
		raster.CullMode = D3D11_CULL_NONE;
		raster.FrontCounterClockwise = FALSE;
		raster.DepthClipEnable = TRUE;

		if (FAILED(device->CreateRasterizerState(&raster, m_Rasterizer.GetAddressOf())))
		{
			m_LogsService.Message("[GpuPipeline] ERROR: Rasterizer state failed.");
			return false;
		}

		D3D11_DEPTH_STENCIL_DESC depth = {};
		depth.DepthEnable = TRUE;
		depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depth.StencilEnable = FALSE;

		if (FAILED(device->CreateDepthStencilState(&depth, m_DepthState.GetAddressOf())))
		{
			m_LogsService.Message("[GpuPipeline] ERROR: Depth state failed.");
			return false;
		}

		D3D11_BLEND_DESC blend = {};
		blend.RenderTarget[0].BlendEnable = FALSE;
		blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		if (FAILED(device->CreateBlendState(&blend, m_BlendState.GetAddressOf())))
		{
			m_LogsService.Message("[GpuPipeline] ERROR: Blend state failed.");
			return false;
		}

		return true;
	}

	auto GpuPipeline::UpdateCamera(ID3D11DeviceContext* context,
		const Matrix& viewProjection) -> void
	{
		if (!context || !m_CameraBuffer) return;

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (SUCCEEDED(context->Map(m_CameraBuffer.Get(), 0,
			D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			std::memcpy(mapped.pData, viewProjection.data(), sizeof(Matrix));
			context->Unmap(m_CameraBuffer.Get(), 0);
		}
	}

	auto GpuPipeline::Bind(ID3D11DeviceContext* context) -> void
	{
		if (!context || !m_IsReady) return;

		const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		context->OMSetBlendState(m_BlendState.Get(), blendFactor, 0xffffffff);
		context->OMSetDepthStencilState(m_DepthState.Get(), 0);
		context->RSSetState(m_Rasterizer.Get());

		context->IASetInputLayout(m_InputLayout.Get());

		ID3D11Buffer* cameraBuffer = m_CameraBuffer.Get();
		context->VSSetShader(m_VertexShader.Get(), nullptr, 0);
		context->VSSetConstantBuffers(0, 1, &cameraBuffer);
		context->PSSetShader(m_PixelShader.Get(), nullptr, 0);

		context->HSSetShader(nullptr, nullptr, 0);
		context->DSSetShader(nullptr, nullptr, 0);
		context->GSSetShader(nullptr, nullptr, 0);
	}

	auto GpuPipeline::Release() -> void
	{
		m_CameraBuffer.Reset();
		m_InputLayout.Reset();
		m_BlendState.Reset();
		m_DepthState.Reset();
		m_Rasterizer.Reset();
		m_PixelShader.Reset();
		m_VertexShader.Reset();

		m_IsReady = false;
	}
}