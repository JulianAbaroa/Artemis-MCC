export module Tables.Object.Type:Projectile;

import std;

export namespace Tables::Object::Type::Projectile
{
	enum class Flags : std::uint32_t
	{
		IsTracking = 0x00000100,
		IsCrit = 0x00400000,
		IsAltFire = 0x00800000,
	};

	struct Projectile
	{
		Flags Flags{};
		std::uint32_t OwnerBipedHandle{};
		std::uint32_t OwnerWeaponHandle{};
	};
}