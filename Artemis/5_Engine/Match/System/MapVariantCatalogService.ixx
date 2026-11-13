export module Engine.Match.System:MapVariantCatalog;

import Common.Log.System;
import Engine.Match.Type;
import std;

export namespace Engine::Match::System
{
    // Lists the .mvar files of Forge that MCC keeps on disk under their GUID names, by the name inside them (Q106, D291).
    // note: Files live in <LocalAppDataLow>/MCC/LocalFiles/<xuid>/HaloReach/Map/, one folder per account. Only the first bytes of each file are
    // read. It touches the disk: a user action or the open of a combo, never every frame, never the game thread.
    class MapVariantCatalogService
    {
    private:
        using path = std::filesystem::path;

        using Log = Common::Log::System::Log;

        using MapVariantEntry = Engine::Match::Type::MapVariantEntry;

    public:
        explicit MapVariantCatalogService(Log& log) : m_Log(log) {}
        ~MapVariantCatalogService() = default;

        MapVariantCatalogService(const MapVariantCatalogService&) = delete;
        auto operator=(const MapVariantCatalogService&) -> MapVariantCatalogService& = delete;

        // return: The files of every account folder of MCC, sorted by name then path. Empty if the folder of MCC is not found. A file that is
        // not a map variant file is skipped with a WARNING.
        auto List() const -> std::vector<MapVariantEntry>;

        // return: The files of every account folder under a LocalFiles folder, sorted as List does.
        auto ListIn(const path& localFiles) const -> std::vector<MapVariantEntry>;

    private:
        Log& m_Log;
    };
}