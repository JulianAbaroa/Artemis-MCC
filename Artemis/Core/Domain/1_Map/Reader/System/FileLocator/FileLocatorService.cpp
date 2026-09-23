module Map.Reader.System;
import :FileLocator;

namespace Map::Reader::System
{
	auto FileLocatorService::OpenMapFile(const std::string& filePath) -> std::ifstream
	{
		if (filePath.empty()) return {};
		return std::ifstream(filePath, std::ios::binary);
	}

	auto FileLocatorService::ResolveExternalCachePath(
		const std::string& mapPath) const -> std::string
	{
		std::string lower;
		lower.reserve(mapPath.size());
		for (char c : mapPath)
		{
			lower.push_back((char)std::tolower((unsigned char)c));
		}

		if (lower.find("shared") != std::string::npos)
		{
			return m_FileStore.GetSharedFilePath();
		}

		if (lower.find("campaign") != std::string::npos)
		{
			return m_FileStore.GetCampaignFilePath();
		}

		return {};
	}
}