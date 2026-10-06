export module Viewer.Palette.System;

import Export.Tick.Type;
import Viewer.Palette.Type;
import Viewer.Style.Type;
import std;

export namespace Viewer::Palette::System
{
    // Builds the table that maps an object handle to its display color on the map.
    // The color depends on the overlay content of the tick. Handles without an entry get the collidable color.
    class PaletteService
    {
    private:
        using Tick = Export::Tick::Type::Tick;
        using Color = Viewer::Style::Type::Color;
        using Entry = Viewer::Palette::Type::Entry;

    public:
        PaletteService() = default;
        ~PaletteService() = default;

        PaletteService(const PaletteService&) = delete;
        auto operator=(const PaletteService&) -> PaletteService& = delete;

        // Rebuilds the table from the tick. Does nothing if that generation was already built.
        // note: A null tick resets the table. If a handle has several entries, the last added wins.
        auto Build(const std::shared_ptr<const Tick>& tick) -> void;

        // Clears the table so the next tick builds it again.
        auto Reset() -> void;

        // return: The color of the handle, or the collidable color if it has no entry.
        auto ColorOf(std::uint32_t handle) const -> Color;

    private:
        std::vector<Entry> m_Entries{};

        std::uint64_t m_LastGeneration{ 0 };
        bool m_IsGenerationSet{ false };
    };
}