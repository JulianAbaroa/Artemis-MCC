module Engine.Control.Input.State;
import :VirtualInput;
import std;

namespace Engine::Control::Input::State
{
    auto VirtualInputStore::SetSource(std::size_t player, LocalInputSource source) -> void
    {
        m_Sources[player].store(source, std::memory_order_release);
    }

    auto VirtualInputStore::GetSource(std::size_t player) const -> LocalInputSource
    {
        return m_Sources[player].load(std::memory_order_acquire);
    }

    auto VirtualInputStore::SetState(std::size_t player, const InputState& state) -> void
    {
        std::lock_guard<std::mutex> lock{ m_StateMutex };
        m_States[player] = state;
    }

    auto VirtualInputStore::CopyState(std::size_t player, InputState& out) const -> void
    {
        std::lock_guard<std::mutex> lock{ m_StateMutex };
        out = m_States[player];
    }

    auto VirtualInputStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock{ m_StateMutex };

        for (std::size_t player{ 0 }; player < m_States.size(); ++player)
        {
            m_Sources[player].store(LocalInputSource::Device, std::memory_order_release);
            m_States[player] = InputState{};
        }
    }
}