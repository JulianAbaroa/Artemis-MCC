module Engine.Control.Input.State;
import :Possession;

import Control.Client.Type;
import std;

namespace
{
    using ::Control::Client::Type::AgentInput;
    using ::Control::Client::Type::AgentInputReducer;
}

namespace Engine::Control::Input::State
{
    auto PossessionStore::Begin(std::size_t playerIndex, std::uint32_t handle) -> bool
    {
        m_TickThread.store(std::this_thread::get_id());

        if (m_Bot && m_Bot->Player == playerIndex && m_Bot->Handle == handle)
        {
            m_Player.store(handle);
            return false;
        }

        m_Bot = ClientDriven{ .Player = playerIndex, .Handle = handle };
        m_Reducer = AgentInputReducer{};
        m_Window = AgentInput{};
        m_IsThreadLogDone = false;
        m_Player.store(handle);
        m_Ticks.store(0);
        m_Delivered.store(0);
        m_Unsupported.store(0);
        m_IsUnitMissing.store(false);
        return true;
    }

    auto PossessionStore::Release() -> void
    {
        m_Bot.reset();
        m_Reducer = AgentInputReducer{};
        m_Window = AgentInput{};
        m_Player.store(0xFFFFFFFF);
    }

    auto PossessionStore::IsTickThread() const -> bool
    {
        return std::this_thread::get_id() == m_TickThread.load();
    }

    auto PossessionStore::GetTickThread() const -> std::thread::id
    {
        return m_TickThread.load();
    }

    auto PossessionStore::Disable() -> bool
    {
        return !m_IsDisabled.exchange(true);
    }

    auto PossessionStore::IsDisabled() const -> bool
    {
        return m_IsDisabled.load();
    }

    auto PossessionStore::MarkThreadsLogged() -> bool
    {
        return !std::exchange(m_IsThreadLogDone, true);
    }

    auto PossessionStore::Find() -> ClientDriven*
    {
        return m_Bot ? &(*m_Bot) : nullptr;
    }

    auto PossessionStore::IsActive() const -> bool
    {
        return m_Player.load() != 0xFFFFFFFF;
    }

    auto PossessionStore::GetReducer() -> AgentInputReducer&
    {
        return m_Reducer;
    }

    auto PossessionStore::SetWindow(const AgentInput& window) -> void
    {
        m_Window = window;
    }

    auto PossessionStore::GetWindow() const -> AgentInput
    {
        return m_Window;
    }

    auto PossessionStore::CountTick(bool isDelivered, bool isUnsupported) -> void
    {
        m_Ticks.fetch_add(1);
        if (isDelivered) m_Delivered.fetch_add(1);
        if (isUnsupported) m_Unsupported.fetch_add(1);
    }

    auto PossessionStore::MarkUnitMissing() -> bool
    {
        return !m_IsUnitMissing.exchange(true);
    }

    auto PossessionStore::GetStatus() const -> PossessStatus
    {
        return PossessStatus{ .Player = m_Player.load(), .Ticks = m_Ticks.load(), .Delivered = m_Delivered.load(),
            .Unsupported = m_Unsupported.load(), .IsUnitMissing = m_IsUnitMissing.load() };
    }
}