module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:MapPass;

import Service.Logs.System;
import Resolved.World.Type;
import std;

export namespace Viewer::Map::System
{
	class MapPass
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using Sbsp = Resolved::World::Type::Sbsp::Sbsp;

		using LogsService = Service::Logs::System::LogsService;

	public:
		explicit MapPass(LogsService& logsService) : m_LogsService(logsService) {}
		~MapPass() = default;

		MapPass(const MapPass&) = delete;
		MapPass& operator=(const MapPass&) = delete;

		auto Upload(ID3D11Device* device, const std::vector<Sbsp>& sbsps) -> void;

		auto IsUploaded() const -> bool;
		auto HasBuffer() const -> bool;

		auto Draw(ID3D11DeviceContext* context) -> void;

		auto Release() -> void;

	private:
		LogsService& m_LogsService;

		ComPtr<ID3D11Buffer> m_VertexBuffer{};
		UINT m_VertexCount{ 0 };
		bool m_IsUploaded{ false };
	};
}