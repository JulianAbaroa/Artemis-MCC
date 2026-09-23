module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Map.System:DynamicPass;

import :Palette;

import Service.Logs.System;
import Export.Tick.Type;
import Viewer.Map.Type;
import std;

export namespace Viewer::Map::System
{
	class DynamicPass
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using Collidables = Export::Tick::Type::Collidables;
		using ObjectBounds = Viewer::Map::Type::ObjectBounds;

		using LogsService = Service::Logs::System::LogsService;

	public:
		explicit DynamicPass(LogsService& logsService) : m_LogsService(logsService) {}
		~DynamicPass() = default;

		DynamicPass(const DynamicPass&) = delete;
		DynamicPass& operator=(const DynamicPass&) = delete;

		auto Upload(ID3D11Device* device, ID3D11DeviceContext* context,
			const std::shared_ptr<const Collidables>& collidables,
			const PaletteService& palette, std::uint32_t selectedHandle,
			std::uint64_t generation) -> void;

		auto GetBounds() const -> std::span<const ObjectBounds>;

		auto Draw(ID3D11DeviceContext* context) -> void;

		auto Release() -> void;

	private:
		LogsService& m_LogsService;

		ComPtr<ID3D11Buffer> m_VertexBuffer{};
		UINT m_Capacity{ 0 };
		UINT m_VertexCount{ 0 };

		std::uint64_t m_LastGeneration{ 0 };
		bool m_HasGeneration{ false };
		std::uint32_t m_LastSelected{ 0xFFFFFFFF };

		std::vector<ObjectBounds> m_Bounds{};

		auto EnsureCapacity(ID3D11Device* device, UINT vertexCapacity) -> bool;
	};
}