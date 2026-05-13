#pragma once

#include <cstdint>

enum class Team : uint8_t;
typedef unsigned int ImU32;

class TeamToColor
{
public:
	static ImU32 TeamColor(Team Team, uint8_t alpha);
};