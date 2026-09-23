module Platform.Input.State;

namespace Platform::Input::State
{
	// We don't use a mutex here because the 
	// HookedGetButtonState() is very sensitive.
	auto InputStore::GetNextRequest() const -> Request
	{
		return m_NextRequest;
	}

	auto InputStore::IsProcessing() const -> bool
	{
		return m_IsProcessing.load();
	}

	auto InputStore::SetNextRequest(Context context, Action action) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_IsProcessing.store(true);
		m_NextRequest = { context, action };
	}

	auto InputStore::SetProcessing(bool processing) -> void
	{
		m_IsProcessing.store(processing);
	}

	auto InputStore::EnqueueRequest(const Request& request, bool uniqueRequest) -> void
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (uniqueRequest && !m_Queue.empty())
		{
			if (m_Queue.back().Action == request.Action) return;
		}

		m_Queue.push(request);
	}

	auto InputStore::DequeueRequest(Request& outRequest) -> bool
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_Queue.empty()) return false;
		outRequest = m_Queue.front();
		m_Queue.pop();
		return true;
	}

	auto InputStore::Cleanup() -> void
	{
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			std::queue<Request> empty;
			std::swap(m_Queue, empty);

			m_NextRequest = { Context::Unknown, Action::Unknown };
		}

		m_IsProcessing.store(false);
	}
}