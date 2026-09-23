export module Map.Reader.State:File;

import std;

export namespace Map::Reader::State
{
	class FileStore
	{
	public:
		FileStore() = default;
		~FileStore() = default;

		auto IsLoaded() const -> bool;
		auto SetLoaded(bool value) -> void;

		auto GetMapFilePath() -> std::string;
		auto SetMapFilePath(std::string path) -> void;

		auto GetCampaignFilePath() const -> std::string;
		auto SetCampaignFilePath(std::string path) -> void;

		auto GetSharedFilePath() const -> std::string;
		auto SetSharedFilePath(std::string path) -> void;

		auto Cleanup() -> void;

	private:
		std::atomic<bool> m_IsLoaded{ false };

		std::string m_MapFilePath{};
		std::string m_CampaingFilePath{};
		std::string m_SharedFilePath{};

		mutable std::mutex m_Mutex{};
	};
}