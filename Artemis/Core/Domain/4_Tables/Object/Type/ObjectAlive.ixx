export module Tables.Object.Type:Alive;

import :Profile;
import :Equipment;
import :Vehicle;
import :Weapon;
import :Biped;
import :Crate;
import :Scenery;
import :Projectile;
import :Multiplayer;
import :Placement;
import Common.Math.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using Profile = Tables::Object::Type::Profile::Profile;
	using Equipment = Tables::Object::Type::Equipment::Equipment;
	using Vehicle = Tables::Object::Type::Vehicle::Vehicle;
	using Weapon = Tables::Object::Type::Weapon::Weapon;
	using Biped = Tables::Object::Type::Biped::Biped;
	using Crate = Tables::Object::Type::Crate::Crate;
	using Scenery = Tables::Object::Type::Scenery::Scenery;
	using Projectile = Tables::Object::Type::Projectile::Projectile;
	using Multiplayer = Tables::Object::Type::Multiplayer::Multiplayer;
	using Placement = Tables::Object::Type::Placement::Placement;

	using Specific = std::variant<std::monostate, Equipment, Vehicle,
		Weapon, Biped, Crate, Scenery, Projectile>;
}

export namespace Tables::Object::Type::Alive
{
	struct Object
	{
		std::uint32_t DatumIndex{};
		std::uintptr_t Address{};

		std::string FourCC{};
		std::string TagName{};
		std::uint16_t HlmtVariant{};

		std::uint32_t Handle{};
		std::uint32_t NextSiblingHandle{};
		std::uint32_t ChildHandle{};
		std::uint32_t ParentHandle{};

		// note: Center of the bounding sphere, world space (offset 0x20). Not the origin of the object (D27).
		Vec3 BoundingCenter{};
		// note: object origin (offset 0x44), local to the parent if parented.
		Vec3 Origin{};
		// note: origin resolved to world space through the parent chain.
		// Zone shapes (Top/Bottom) are measured from here, not from BoundingCenter.
		Vec3 WorldOrigin{};
		Vec3 Forward{};
		Vec3 Up{};

		Vec3 LinearVelocity{};
		Vec3 AngularVelocity{};

		float BoundingRadius{};
		float DamageReceived{};

		::Profile Profile{};

		// note: Only objects that have the multiplayer block.
		std::optional<::Multiplayer> Multiplayer{};
		// note: Only objects placed by the map variant (obje 0x1C). Holds the Forge settings, Physics included.
		std::optional<::Placement> Placement{};
		Specific Specific{};
	};
}