#pragma once

#include "Core/Types/Team.h"

#include <array>

struct Self
{
    uint32_t Handle{ 0xFFFFFFFF };
    uint32_t BipedHandle{ 0xFFFFFFFF };
    bool IsAlive{ false };
    Team Team{};

    std::array<float, 3> Position{};
    std::array<float, 3> Forward{};
    std::array<float, 3> Right{};
    std::array<float, 3> Up{};
};