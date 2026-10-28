export module Map.Reader.System:FileLocator;

import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
    // Opens the map file and finds the external cache maps that a map can point to.
    class FileLocatorService
    {
    private:
        using FileStore = Map::Reader::State::FileStore;

    public:
        explicit FileLocatorService(FileStore& fileStore) :
            m_FileStore(fileStore) {}
        ~FileLocatorService() = default;

        // Opens a map file for binary reading.
        // return: A stream that is not open if the path is empty or the file cannot be opened.
        auto OpenMapFile(const std::string& filePath) -> std::ifstream;

        // Gets the path of the external cache map that a map path refers to.
        // param mapPath: Path stored in an external cache reference. Its name tells if it is the shared or the campaign map.
        // return: The stored path of that map, or empty if it is neither.
        auto ResolveExternalCachePath(
            const std::string& mapPath) const -> std::string;

    private:
        FileStore& m_FileStore;
    };
}