#pragma once

#include <cstdint>

struct LivePlayer;
class CopyableField;

class HandleDrawer
{
public:
	static void DrawU32(const char* label, uint32_t handle, 
		const LivePlayer& player, CopyableField& field);
};