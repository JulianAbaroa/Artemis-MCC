export module Common.ZoneShape.Type;

import std;

export namespace Common::ZoneShape::Type
{
	enum class Kind : std::uint8_t
	{
		None = 0x00,
		//Sphere = 0x01,	// Unused.
		Cylinder = 0x02,
		Box = 0x03,
	};

	struct ZoneShape
	{
		float Radius{}; // Or Width for box.
		float Length{};
		float Top{};
		float Bottom{};
		Kind Kind{ Kind::None };
	};
}