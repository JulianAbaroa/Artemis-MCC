module Map.Reader.System;
import :MapLoader;

// This '.map' reader was based on:
// Assembly: https://github.com/xboxchaos/assembly
// Reclaimer: https://github.com/Gravemind2401/Reclaimer

// Note: offset mask [3] is not yet used. 
// It possibly corresponds to locales, that requires 
// its own To...Offset() conversion function.

namespace Map::Reader::System
{
	auto MapLoaderService::LoadMap(const std::string& filePath) -> void
	{
		m_FileStore.SetMapFilePath(filePath);

		std::ifstream file = m_FileLocatorService.OpenMapFile(filePath);
		if (!file.is_open())
		{
			m_LogsService.Message("[MapLoaderService] ERROR: Could not open map file.");
			m_FileStore.SetLoaded(false);
			return;
		}

		bool isValid = false;

		auto headerResult = m_HeaderService.Read(file);
		if (headerResult)
		{
			m_HeaderInfo = *headerResult;

			isValid = m_TagIndexService.Read(
				file, m_HeaderInfo.IndexHeaderFileOffset) &&
				m_FileNamesService.Read(file,
					m_HeaderInfo.FileTableCount,
					m_HeaderInfo.FileIndexTableOffset,
					m_HeaderInfo.FileTableOffset,
					m_HeaderInfo.FileTableSize);
		}

		if (isValid)
		{
			m_LogsService.Message("[MapLoaderService] INFO: Map loaded.");
		}
		else
		{
			m_LogsService.Message("[MapLoaderService] ERROR: Failed to load map.");
		}

		m_FileStore.SetLoaded(isValid);
	}

	auto MapLoaderService::Cleanup() -> void
	{
		m_HeaderInfo = {};

		m_FileStore.Cleanup();
		m_TagIndexStore.Cleanup();

		m_LogsService.Message("[MapLoaderService] INFO: Cleanup completed.");
	}
}