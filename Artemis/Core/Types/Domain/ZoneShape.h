#pragma once

#include <cstdint>

enum class ShapeType : uint8_t
{
	None = 0x00,
	//Sphere = 0x01,
	Cylinder = 0x02,
	Box = 0x03,
};

struct ZoneShape
{
	float Radius; // Or Width for box.
	float Length;
	float Top;
	float Bottom;
	ShapeType ShapeType;
};