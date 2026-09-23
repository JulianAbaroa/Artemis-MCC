export module Tables.Interaction.Type:Alive;

import Common.Math.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
}

export namespace Tables::Interaction::Type::Alive
{
	enum class Kind : std::uint8_t
	{
		None = 0x00,
		GrabWeapon = 0x02,
		GrabArmorAbility = 0x03,
		TakeHealthStation = 0x04,
		EnterVehicle = 0x06,
		Hijack = 0x09,
		GrabObjective = 0x0A,
	};

	enum class Detail : std::uint8_t
	{
		None = 0xFF,
		ZeroSeat = 0x00,
		FirstSeat = 0x01,
		SecondSeat = 0x02,
		ThirdSeat = 0x03,
		FourthSeat = 0x04,
		FifthSeat = 0x05,
		GrabWeapon = 0x01,
		ChangeWeapon = 0x02
	};

	struct Interaction
	{
		// Objects.
		Kind Kind = Kind::None;
		Detail InteractionSlotID = Detail::None;
		std::uint32_t TargetObjectHandle = 0xFFFFFFFF;

		// Players: Melee.
		std::uint8_t IsMeleeAvailable = 0x00;
		std::uint32_t MeleeTargetHandle = 0xFFFFFFFF;

		// Players: Aim.
		std::uint8_t IsAimAvailable = 0x00;
		std::uint8_t ModelPart = 0x00; // TODO: Connect with mode tag data.
		std::uint32_t AimTargetHandle = 0xFFFFFFFF;
		std::uint32_t AimTargetSlotID = 0xFFFFFFFF;
		Vec3 AimHitLocalPosition{};
	};
}