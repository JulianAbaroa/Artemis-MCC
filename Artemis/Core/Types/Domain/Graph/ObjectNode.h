#pragma once

#include "../Object/ObjectProfile.h"
#include "../Object/LiveObject.h"
#include "../Player/LivePlayer.h"
#include <cstdint>
#include <string>
#include <vector>

// Representation of the relationship between father-child-sibling objects.
struct ObjectNode
{
	uint32_t Handle;
	uint32_t ParentHandle;
	std::vector<uint32_t> ChildrenHandles;
};