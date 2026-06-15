#pragma once

#include <cstdint>

enum class ObjectRole : uint8_t;
struct ImVec4;

class RoleToColor
{
public:
	static ImVec4 FromRole(ObjectRole role);
};