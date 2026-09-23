module Service.Logs.State;

namespace Service::Logs::State
{
	auto LogsStore::PushBack(Entry entry) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Logs.push_back(std::move(entry));
	}

	auto LogsStore::TrimToSize(int size) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_Logs.size() > size)
		{
			m_Logs.pop_front();
		}
	}

	auto LogsStore::ForEachLog(std::function<void(
		const Entry&)> callback) const -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		for (const auto& log : m_Logs)
		{
			callback(log);
		}
	}

	auto LogsStore::ClearLogs() -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Logs.clear();
	}

	auto LogsStore::GetMaxCapacity() const -> int
	{
		return m_MaxCapacity.load();
	}

	auto LogsStore::GetLogAt(std::size_t index) const -> Entry
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (index >= m_Logs.size()) return {};
		return m_Logs[index];
	}

	auto LogsStore::GetTotalLogs() const -> std::size_t
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Logs.size();
	}

	auto LogsStore::RemoveIf(std::function<bool(const Entry&)> predicate) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		m_Logs.erase(std::remove_if(m_Logs.begin(),
			m_Logs.end(), predicate), m_Logs.end());
	}
}