module Bridge.Codec.Flat.State;

import std;

namespace Bridge::Codec::Flat::State
{
    auto FlatStore::Push(FlatRequest request) -> void
    {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Pending = std::move(request);
        }

        m_Condition.notify_one();
    }

    auto FlatStore::WaitPop(milliseconds timeout) -> std::optional<FlatRequest>
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_Condition.wait_for(lock, timeout, [&] { return m_Pending.has_value(); });

        std::optional<FlatRequest> request{ std::move(m_Pending) };
        m_Pending.reset();
        return request;
    }

    auto FlatStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Pending.reset();
    }
}