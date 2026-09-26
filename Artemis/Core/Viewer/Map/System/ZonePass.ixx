module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:ZonePass;

import :Palette;

import Service.Logs.System;
import Export.Tick.Type;
import std;

export namespace Viewer::Map::System
{
	class ZonePass
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using Tick = Export::Tick::Type::Tick;

		using LogsService = Service::Logs::System::LogsService;

	public:
		explicit ZonePass(LogsService& logsService) : m_LogsService(logsService) {}
		~ZonePass() = default;

		ZonePass(const ZonePass&) = delete;
		ZonePass& operator=(const ZonePass&) = delete;

		auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
			const std::shared_ptr<const Tick>& tick, const PaletteService& palette) -> void;

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