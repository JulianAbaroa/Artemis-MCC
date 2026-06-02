#pragma once

#include <cstdint>

enum class Team : uint8_t;
typedef unsigned int ImU32;
struct ImVec4;

class TeamToColor
{
public:
	static ImU32 TeamColorU32(Team team, uint8_t alpha);
	static ImVec4 TeamColorVec4(Team team, float alpha);
};