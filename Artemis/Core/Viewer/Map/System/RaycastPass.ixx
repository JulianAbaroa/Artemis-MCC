module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:RaycastPass;

import Service.Logs.System;
import Export.Tick.Type;
import std;

export namespace Viewer::Map::System
{
	class RaycastPass
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using Raycasts = Export::Tick::Type::Raycasts;

		using LogsService = Service::Logs::System::LogsService;

	public:
		explicit RaycastPass(LogsService& logsService) : m_LogsService(logsService) {}
		~RaycastPass() = default;

		RaycastPass(const RaycastPass&) = delete;
		RaycastPass& operator=(const RaycastPass&) = delete;

		auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
			const std::shared_ptr<const Raycasts>& raycasts, std::uint64_t generation) -> void;

		auto Draw(ID3D11DeviceContext* context) -> void;

		auto Release() -> void;

	private:
		LogsService& m_LogsService;

		ComPtr<ID3D11Buffer> m_VertexBuffer{};
		UINT m_Capacity{ 0 };
		UINT m_VertexCount{ 0 };

		std::uint64_t m_LastGeneration{ 0 };
		bool m_HasGeneration{ false };
	};
}