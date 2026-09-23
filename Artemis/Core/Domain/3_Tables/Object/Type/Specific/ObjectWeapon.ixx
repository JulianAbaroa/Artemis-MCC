export module Tables.Object.Type:Weapon;

import Common.Team.Type;
import std;

namespace
{
	using Team = Common::Team::Type::Team;
}

export namespace Tables::Object::Type::Weapon
{
	enum class ActionState : std::uint8_t
	{
		Idle = 0x00,
		Firing = 0x02,
		Meleeing = 0x60,
		Zoomed = 0x80,
	};

	struct Weapon
	{
		// Ammo-related
		float TotalHeat{};
		float TotalEnergy{};
		std::uint16_t TotalAmmo{};
		std::uint16_t CurrentAmmo{};

		// State-related.
		std::uint8_t IsFiring{};
		std::uint8_t IsReloading{};
		std::uint8_t ChargeProgress{};
		ActionState ActionState{};

		std::optional<Team> Team{};
	};
}