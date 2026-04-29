#pragma once

#include "External/imgui/imgui.h"
#include <filesystem>
#include <vector>
#include <string>

// Visual definitions for ImGui elements.
struct PhaseUI
{
	const char* Name;
	ImVec4 Color;
};

struct LogFilterState
{
	std::string SearchStr;
	bool IsFiltering;
};