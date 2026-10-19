module Platform.Input.State;
import :PlayerAction;

import std;

namespace
{
    using Platform::Input::Type::k_ActionButtonsOffset;
    using Platform::Input::Type::k_MaxActionPlayers;
    using Platform::Input::Type::k_PlayerActionSize;

    // Full turn in radians.
    constexpr float k_TwoPi{ 6.28318531f };
}

namespace Platform::Input::State
{
    auto PlayerActionStore::Publish(const PlayerActions& actions) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Latest = actions;

        for (std::size_t player{ 0 }; player < m_Watches.size(); ++player)
        {
            if ((actions.Mask & (1u << player)) == 0) continue;

            const auto& action{ actions.Actions[player] };
            auto& watch{ m_Watches[player] };

            std::uint64_t buttons{ 0 };
            std::memcpy(&buttons, action.Data.data() + k_ActionButtonsOffset, sizeof(buttons));
            watch.SeenButtons |= buttons;

            if (!watch.HasReference)
            {
                watch.Reference = action;
                watch.HasReference = true;
                continue;
            }

            for (std::size_t offset{ 0 }; offset < k_PlayerActionSize; ++offset)
            {
                if (action.Data[offset] == watch.Reference.Data[offset]) continue;

                watch.IsChanged[offset] = true;
                watch.LastChanged.Data[offset] = action.Data[offset];
            }
        }
    }

    auto PlayerActionStore::CopyLatest(PlayerActions& out) const -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        out = m_Latest;
    }

    auto PlayerActionStore::CopyWatch(std::size_t playerIndex, ActionWatch& out) const -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        out = m_Watches[playerIndex];
    }

    auto PlayerActionStore::ClearWatches() -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Watches.fill(ActionWatch{});
    }

    auto PlayerActionStore::SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Overrides[playerIndex] = actionOverride;
    }

    auto PlayerActionStore::CopyOverrides(std::array<std::optional<ActionOverride>, k_MaxActionPlayers>& out) const -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        out = m_Overrides;
    }

    auto PlayerActionStore::AdvanceOverrideYaw(std::size_t playerIndex, float radians) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };

        auto& actionOverride{ m_Overrides[playerIndex] };
        if (!actionOverride) return;

        float yaw{ std::fmod(actionOverride->Yaw + radians, k_TwoPi) };
        if (yaw < 0.0f) yaw += k_TwoPi;

        actionOverride->Yaw = yaw;
    }

    auto PlayerActionStore::RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void
    {
        if (playerIndex >= k_MaxActionPlayers) return;

        std::lock_guard<std::mutex> lock{ m_Mutex };

        ClientRequests& pending{ m_ClientRequests[playerIndex] };
        pending.IsWeaponSwitch = pending.IsWeaponSwitch || requests.IsWeaponSwitch;
        pending.IsGrenadeSwitch = pending.IsGrenadeSwitch || requests.IsGrenadeSwitch;
        pending.IsZoom = pending.IsZoom || requests.IsZoom;
    }

    auto PlayerActionStore::TakeClientRequests(std::array<ClientRequests, k_MaxActionPlayers>& out) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        out = m_ClientRequests;
        m_ClientRequests.fill(ClientRequests{});
    }

    auto PlayerActionStore::Cleanup() -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Latest = PlayerActions{};
        m_Watches.fill(ActionWatch{});
        m_Overrides.fill(std::nullopt);
        m_ClientRequests.fill(ClientRequests{});
    }
}