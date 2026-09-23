export module Tables.Player.Type:Alive;

import Common.Team.Type;
import Common.Math.Type;
import std;

namespace
{
	using Team = Common::Team::Type::Team;
	using Vec3 = Common::Math::Type::Vec3;
}

export namespace Tables::Player::Type::Alive
{
	enum class ConnectionState : std::uint8_t
	{
		Connected = 0x01,
		Disconnected = 0x02,
		Connecting = 0x08,
	};

	struct Player
	{
		std::uint32_t Handle{};
		std::uintptr_t Address{};

		ConnectionState ConnectionState{};
		Team Team{};

		std::string Gamertag{};
		std::string Tag{};

		Vec3 WeaponPosition{};
		Vec3 WeaponForward{};
		Vec3 AimOffset{};

		std::uint32_t PrimaryWeaponHandle{};
		std::uint32_t SecondaryWeaponHandle{};
		std::uint32_t ObjectiveHandle{};

		std::uint32_t AliveBipedHandle{};
		std::uint32_t DeadBipedHandle{};
		std::uint32_t CurrentBipedHandle{};
	};
}