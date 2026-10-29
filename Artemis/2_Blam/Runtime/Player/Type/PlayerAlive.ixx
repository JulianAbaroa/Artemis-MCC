export module Blam.Runtime.Player.Type:Alive;

import Common.Team.Type;
import Common.Math.Type;
import std;

namespace
{
    using Common::Team::Type::Team;
    using Common::Math::Type::Vec3;
}

export namespace Blam::Runtime::Player::Type::Alive
{
    enum class ConnectionState : std::uint8_t
    {
        Connected = 0x01,
        Disconnected = 0x02,
        Connecting = 0x08,
    };

    // Snapshot of one entry of the engine player table.
    struct AlivePlayer
    {
        // The salt is in the high 16 bits and the table index in the low 16 bits.
        std::uint32_t Handle{};

        ConnectionState ConnectionState{};
        Team Team{};

        std::string Gamertag{};
        std::string Tag{};

        Vec3 CameraPosition{};
        Vec3 CameraForward{};

        // note: The aim hit of the raycast replaced it. Kept as raw engine data.
        Vec3 AimOffset{};

        std::uint32_t PrimaryWeaponHandle{};
        std::uint32_t SecondaryWeaponHandle{};
        std::uint32_t ObjectiveHandle{};

        std::uint32_t AliveBipedHandle{};
        std::uint32_t DeadBipedHandle{};
        std::uint32_t CurrentBipedHandle{};

        // Local player (split-screen input user, 0 to 3) the player is, or empty when it is not local.
        std::optional<std::uint8_t> LocalIndex{};
    };

    // Players by handle.
    struct PlayerTable : std::unordered_map<std::uint32_t, AlivePlayer>
    {
        using unordered_map::unordered_map;
    };
}