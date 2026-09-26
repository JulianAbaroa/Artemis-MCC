export module Common.State.System;

import std;

export namespace Common::State::System
{
	template <typename T>
	class Snapshot
	{
	public:
		Snapshot() = default;
		~Snapshot() = default;

        auto Publish(T value) -> void
        {
            auto snap = std::make_shared<const T>(std::move(value));
            m_Value.store(snap, std::memory_order_release);
        }

        auto Acquire() const -> std::shared_ptr<const T>
        {
            return m_Value.load(std::memory_order_acquire);
        }

        auto Cleanup() -> void
        {
            m_Value.store(nullptr, std::memory_order_release);
        }

    private:
        std::atomic<std::shared_ptr<const T>> m_Value{};
	};
}