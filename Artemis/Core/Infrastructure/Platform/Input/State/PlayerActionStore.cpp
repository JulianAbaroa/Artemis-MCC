module Platform.Input.State;
import :PlayerAction;

import std;

namespace
{
    using Platform::Input::Type::k_ActionButtonsOffset;
}

namespace Platform::Input::State
{
    auto PlayerActionStore::Publish(const PlayerActions& actions) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Latest = actions;

        for (std::size_t player{ 0 }; player < m_SeenButtons.size(); ++player)
        {
            if ((actions.Mask & (1u << player)) == 0) continue;

            std::uint64_t buttons{ 0 };
            std::memcpy(&buttons, actions.Actions[player].Data.data() + k_ActionButtonsOffset, sizeof(buttons));
            m_SeenButtons[player] |= buttons;
        }
    }

    auto PlayerActionStore::CopyLatest(PlayerActions& out) const -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        out = m_Latest;
    }

    auto PlayerActionStore::GetSeenButtons(std::size_t playerIndex) const -> std::uint64_t
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_SeenButtons[playerIndex];
    }

    auto PlayerActionStore::ClearSeenButtons() -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_SeenButtons.fill(0);
    }

    auto PlayerActionStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Latest = PlayerActions{};
        m_SeenButtons.fill(0);
    }
}