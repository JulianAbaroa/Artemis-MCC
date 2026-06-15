#pragma once

#include <cstdint>
#include <vector>

struct ObjectNode
{
	uint32_t Handle;
	uint32_t ParentHandle;
	std::vector<uint32_t> ChildrenHandles;
};