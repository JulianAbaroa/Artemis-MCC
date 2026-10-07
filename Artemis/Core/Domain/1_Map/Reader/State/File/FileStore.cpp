module Map.Reader.State;
import :File;

namespace Map::Reader::State
{
    auto FileStore::IsLoaded() const -> bool
    {
        return m_IsLoaded.load();
    }

    auto FileStore::SetLoaded(bool value) -> void
    {
        m_IsLoaded.store(value);
    }

    auto FileStore::GetMapFilePath() const -> std::string
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_MapFilePath;
    }

    auto FileStore::SetMapFilePath(std::string path) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_MapFilePath = path;
    }

    auto FileStore::GetCampaignFilePath() const -> std::string
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_CampaignFilePath;
    }

    auto FileStore::SetCampaignFilePath(std::string path) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_CampaignFilePath = path;
    }

    auto FileStore::GetSharedFilePath() const -> std::string
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_SharedFilePath;
    }

    auto FileStore::SetSharedFilePath(std::string path) -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_SharedFilePath = path;
    }

    auto FileStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_MapFilePath.clear();
        m_CampaignFilePath.clear();
        m_SharedFilePath.clear();

        m_IsLoaded.store(false);
    }
}