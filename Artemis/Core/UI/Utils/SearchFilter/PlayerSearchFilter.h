#pragma once

#include "Core/Types/Player/LivePlayer.h"

#include <string>

class PlayerSearchFilter
{
public:
    void DrawSearchBar() const;
    bool PassesFilter(const LivePlayer& player) const;
    bool IsActive() const { return m_Query[0] != '\0'; }

private:
    mutable char m_Query[128] = "";
};