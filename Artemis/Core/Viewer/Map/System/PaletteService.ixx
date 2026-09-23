export module Viewer.Map.System:Palette;

import Common.Team.Type;
import Export.Tick.Type;
import Viewer.Map.Type;
import std;

export namespace Viewer::Map::System
{
	class PaletteService
	{
	private:
		using Tick = Export::Tick::Type::Tick;
		using Team = Common::Team::Type::Team;
		using Color = Viewer::Map::Type::Color;

	public:
		PaletteService() = default;
		~PaletteService() = default;

		PaletteService(const PaletteService&) = delete;
		PaletteService& operator=(const PaletteService&) = delete;

		auto Build(const std::shared_ptr<const Tick>& tick) -> void;

		auto Reset() -> void;

		auto ColorOf(std::uint32_t handle) const -> Color;

	private:
		std::unordered_set<std::uint32_t> m_HealthHandles{};
		std::unordered_set<std::uint32_t> m_AffordanceHandles{};
		std::unordered_map<std::uint32_t, Color> m_FixtureColors{};

		std::uint64_t m_LastGeneration{ 0 };
		bool m_HasGeneration{ false };
	};
}