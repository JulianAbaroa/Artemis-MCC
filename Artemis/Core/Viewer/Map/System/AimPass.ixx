module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:AimPass;

import Service.Logs.System;
import Environment.Aim.Type;
import std;

export namespace Viewer::Map::System
{
	class AimPass
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		using LogsService = Service::Logs::System::LogsService;
		using Aims = Environment::Aim::Type::Aims;

	public:
		explicit AimPass(LogsService& logsService) : m_LogsService(logsService) {}
		~AimPass() = default;

		AimPass(const AimPass&) = delete;
		AimPass& operator=(const AimPass&) = delete;

		auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
			const std::shared_ptr<const Aims>& aims, std::uint32_t selectedHandle,
			std::uint64_t generation) -> void;

		auto Draw(ID3D11DeviceContext* context) -> void;

		auto Release() -> void;

	private:
		LogsService& m_LogsService;

		ComPtr<ID3D11Buffer> m_FillBuffer{};
		ComPtr<ID3D11Buffer> m_WireBuffer{};
		UINT m_FillCapacity{ 0 };
		UINT m_WireCapacity{ 0 };
		UINT m_FillCount{ 0 };
		UINT m_WireCount{ 0 };

		ComPtr<ID3D11DepthStencilState> m_NoDepthState{};
		ComPtr<ID3D11BlendState> m_FillBlendState{};

		std::uint64_t m_LastGeneration{ 0 };
		std::uint32_t m_LastSelected{ 0 };
		bool m_HasUpload{ false };
	};
}