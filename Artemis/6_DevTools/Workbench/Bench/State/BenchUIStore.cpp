module DevTools.Workbench.Bench.State;

namespace DevTools::Workbench::Bench::State
{
    auto BenchUIStore::SetFolder(path folder) -> void
    {
        m_Folder = std::move(folder);
        m_IsEnabled.store(!m_Folder.empty(), std::memory_order_release);
    }

    auto BenchUIStore::GetFolder() const -> const path&
    {
        return m_Folder;
    }

    auto BenchUIStore::IsEnabled() const -> bool
    {
        return m_IsEnabled.load(std::memory_order_acquire);
    }

    auto BenchUIStore::GetRun() -> BenchRun&
    {
        return m_Run;
    }

    auto BenchUIStore::SetStatus(std::string status) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Status = std::move(status);
    }

    auto BenchUIStore::GetStatus() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Status;
    }

    auto BenchUIStore::GetScenarioFiles() -> std::vector<path>&
    {
        return m_ScenarioFiles;
    }

    auto BenchUIStore::SetSelectedScenario(std::size_t index) -> void
    {
        m_SelectedScenario = index;
    }

    auto BenchUIStore::GetSelectedScenario() const -> std::size_t
    {
        return m_SelectedScenario;
    }
}