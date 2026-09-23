export module Service.Logs.State;

import Service.Logs.Type;
import std;

export namespace Service::Logs::State
{
	class LogsStore
	{
	private:
		using Entry = Service::Logs::Type::Entry;

	public:
		LogsStore() = default;
		~LogsStore() = default;

		auto PushBack(Entry entry) -> void;
		auto TrimToSize(int size) -> void;

		auto ForEachLog(std::function<void(const Entry&)> callback) const -> void;
		auto ClearLogs() -> void;

		auto GetMaxCapacity() const -> int;

		auto GetLogAt(std::size_t index) const -> Entry;
		auto GetTotalLogs() const -> std::size_t;

		auto RemoveIf(std::function<bool(const Entry&)> predicate) -> void;

	private:
		std::deque<Entry> m_Logs{};
		mutable std::mutex m_Mutex{};

		const std::atomic<int> m_MaxCapacity{ 500 };
	};
}