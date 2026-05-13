#pragma once

// Types.
#include "Core/Types/Domain/Object/LiveObject.h"

#include <string>

class ObjectSearchFilter
{
public:
    void DrawSearchBar() const;
    bool PassesFilter(const LiveObject& object) const;
    bool IsActive() const { return m_Query[0] != '\0'; }

private:
    mutable char m_Query[128] = "";
};