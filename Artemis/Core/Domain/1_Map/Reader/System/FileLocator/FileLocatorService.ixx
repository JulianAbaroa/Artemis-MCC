export module Map.Reader.System:FileLocator;

import Map.Reader.State;
import std;

export namespace Map::Reader::System
{
	class FileLocatorService
	{
	private:
		using FileStore = Map::Reader::State::FileStore;

	public:
		explicit FileLocatorService(FileStore& fileStore) :
			m_FileStore(fileStore) {}
		~FileLocatorService() = default;

		auto OpenMapFile(const std::string& filePath) -> std::ifstream;

		auto ResolveExternalCachePath(
			const std::string& mapPath) const -> std::string;

	private:
		FileStore& m_FileStore;
	};
}