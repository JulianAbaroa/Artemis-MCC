module Platform.Host.State;

import std;

namespace
{
    using Platform::Host::Type::k_MaxLocalUsers;
}

namespace Platform::Host::State
{
    LocalUserStore::LocalUserStore()
    {
        // Random base so the XUIDs do not collide with real users. Same idea as AlphaRing.
        std::random_device device{};
        const std::uint64_t base{ (static_cast<std::uint64_t>(device()) << 32) ^ static_cast<std::uint64_t>(device()) };

        for (std::size_t user{ 1 }; user < k_MaxLocalUsers; ++user)
        {
            m_Xuids[user] = base + user;

            const std::wstring name{ std::format(L"Artemis {}", user) };
            std::ranges::copy(name, m_Names[user].begin());
        }
    }

    auto LocalUserStore::SetRequestedCount(std::size_t count) -> void
    {
        m_RequestedCount.store(std::clamp<std::size_t>(count, 1, k_MaxLocalUsers), std::memory_order_relaxed);
    }

    auto LocalUserStore::GetRequestedCount() const -> std::size_t
    {
        return m_RequestedCount.load(std::memory_order_relaxed);
    }

    auto LocalUserStore::GetXuid(std::size_t user) const -> std::uint64_t
    {
        return m_Xuids[user];
    }

    auto LocalUserStore::GetName(std::size_t user) const -> std::wstring_view
    {
        return std::wstring_view{ m_Names[user].data() };
    }

    auto LocalUserStore::FindUser(std::uint64_t xuid) const -> std::optional<std::size_t>
    {
        for (std::size_t user{ 1 }; user < k_MaxLocalUsers; ++user)
        {
            if (m_Xuids[user] == xuid) return user;
        }

        return std::nullopt;
    }

    auto LocalUserStore::SetPrimaryXuid(std::uint64_t xuid) -> void
    {
        m_PrimaryXuid.store(xuid, std::memory_order_relaxed);
    }

    auto LocalUserStore::GetPrimaryXuid() const -> std::uint64_t
    {
        return m_PrimaryXuid.load(std::memory_order_relaxed);
    }

    auto LocalUserStore::GetProfile(std::size_t user) -> PlayerProfile&
    {
        return m_Profiles[user];
    }

    auto LocalUserStore::GetGamepadMapping(std::size_t user) -> GamepadMapping&
    {
        return m_GamepadMappings[user];
    }

    auto LocalUserStore::MarkReported(std::size_t user) -> bool
    {
        return !m_IsReported[user].exchange(true, std::memory_order_relaxed);
    }

    auto LocalUserStore::IsReported(std::size_t user) const -> bool
    {
        return m_IsReported[user].load(std::memory_order_relaxed);
    }

    auto LocalUserStore::Cleanup() -> void
    {
        for (auto& isReported : m_IsReported)
        {
            isReported.store(false, std::memory_order_relaxed);
        }
    }
}