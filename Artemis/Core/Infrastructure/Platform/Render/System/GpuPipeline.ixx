module;

#include <d3d11.h>
#include <wrl/client.h>

export module Platform.Render.System:GpuPipeline;

import Service.Logs.System;
import Platform.Render.Type;
import std;

export namespace Platform::Render::System
{
	class GpuPipeline
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		using LogsService = Service::Logs::System::LogsService;

	public:
		using Vertex = Platform::Render::Type::Vertex;
		using Matrix = std::array<float, 16>;

		static constexpr UINT k_VertexStride = sizeof(Vertex);

		explicit GpuPipeline(LogsService& logsService) : m_LogsService(logsService) {}
		~GpuPipeline() = default;

		GpuPipeline(const GpuPipeline&) = delete;
		GpuPipeline& operator=(const GpuPipeline&) = delete;

		auto Initialize(ID3D11Device* device) -> bool;
		auto IsReady() const -> bool;

		auto UpdateCamera(ID3D11DeviceContext* context, const Matrix& viewProjection) -> void;

		auto Bind(ID3D11DeviceContext* context) -> void;

		auto Release() -> void;

	private:
		LogsService& m_LogsService;

		bool m_IsReady{ false };

		ComPtr<ID3D11VertexShader> m_VertexShader{};
		ComPtr<ID3D11PixelShader> m_PixelShader{};
		ComPtr<ID3D11InputLayout> m_InputLayout{};

		ComPtr<ID3D11Buffer> m_CameraBuffer{};

		ComPtr<ID3D11RasterizerState> m_Rasterizer{};
		ComPtr<ID3D11DepthStencilState> m_DepthState{};
		ComPtr<ID3D11BlendState> m_BlendState{};

		auto CompileShaders(ID3D11Device* device) -> bool;
		auto CreateBuffers(ID3D11Device* device) -> bool;
		auto CreateStates(ID3D11Device* device) -> bool;
	};
}