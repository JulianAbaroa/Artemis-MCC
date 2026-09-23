export module Tables.Object.Type:Biped;

import Common.Math.Type;
import std;

namespace
{
	using Vec2 = Common::Math::Type::Vec2;
	using Vec3 = Common::Math::Type::Vec3;
}

export namespace Tables::Object::Type::Biped
{
	enum class ZoomLevel : std::uint8_t
	{
		None = 0xFF,
		Zoom1 = 0x00,
		Zoom2 = 0x01,
	};

	struct Biped
	{
		// Movement.
		Vec2 MovementDirection{};
		Vec3 SurfaceNormal{};
		std::uint32_t GroundObjectHandle{};
		std::uint16_t MaterialIndex{};

		// States.
		std::uint8_t IsCrouched{};
		std::uint8_t IsGrounded{};
		std::uint8_t IsAbilityActive{};
		ZoomLevel ZoomLevel{};

		// Damager.
		std::uint32_t DamagerBipedHandle{};
		std::uint32_t DamagerPlayerHandle{};
	};
}