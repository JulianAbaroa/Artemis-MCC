export module Platform.Host.State;

import Platform.Host.Type;
import std;

namespace
{
    using Platform::Host::Type::k_MaxLocalUsers;
}

export namespace Platform::Host::State
{
    // Local users Artemis adds next to the real one (split-screen): how many, their XUID, name, profile and gamepad mapping.
    // note: User 0 is always the real one of MCC. Users 1 to 3 are Artemis users.
    // note: The count and the reported flags are thread-safe. Profiles and mappings are written by the host hooks only.
    class LocalUserStore
    {
    private:
        using PlayerProfile = Platform::Host::Type::PlayerProfile;
        using GamepadMapping = Platform::Host::Type::GamepadMapping;

    public:
        // Generates the XUIDs and names of the Artemis users.
        LocalUserStore();
        ~LocalUserStore() = default;

        LocalUserStore(const LocalUserStore&) = delete;
        auto operator=(const LocalUserStore&) -> LocalUserStore& = delete;

        // Sets how many local users the engine sees, the real one included.
        // note: Clamped to 1..k_MaxLocalUsers. 1 leaves the game untouched. Read when a match starts.
        auto SetRequestedCount(std::size_t count) -> void;
        auto GetRequestedCount() const -> std::size_t;

        auto GetXuid(std::size_t user) const -> std::uint64_t;
        auto GetName(std::size_t user) const -> std::wstring_view;

        // return: The Artemis user that owns the XUID, nullopt for any other XUID.
        auto FindUser(std::uint64_t xuid) const -> std::optional<std::size_t>;

        // XUID of the real user 0, seen when MCC reports it.
        // note: The profile and the mapping of the Artemis users are copied from that user.
        auto SetPrimaryXuid(std::uint64_t xuid) -> void;
        auto GetPrimaryXuid() const -> std::uint64_t;

        auto GetProfile(std::size_t user) -> PlayerProfile&;
        auto GetGamepadMapping(std::size_t user) -> GamepadMapping&;

        // Marks that the engine asked for the user.
        // return: True the first time since the last cleanup.
        auto MarkReported(std::size_t user) -> bool;
        auto IsReported(std::size_t user) const -> bool;

        // Clears the reported flags. The count and the identities stay.
        auto Cleanup() -> void;

    private:
        static constexpr std::size_t k_NameCapacity{ 16 };

        std::atomic<std::size_t> m_RequestedCount{ 1 };
        std::atomic<std::uint64_t> m_PrimaryXuid{ 0 };

        std::array<std::uint64_t, k_MaxLocalUsers> m_Xuids{};
        std::array<std::array<wchar_t, k_NameCapacity>, k_MaxLocalUsers> m_Names{};
        std::array<PlayerProfile, k_MaxLocalUsers> m_Profiles{};
        std::array<GamepadMapping, k_MaxLocalUsers> m_GamepadMappings{};
        std::array<std::atomic<bool>, k_MaxLocalUsers> m_IsReported{};
    };
}