#pragma once

#include "../MapTag.h"

#include "Generated/Zone/ZoneObject.h"

template <typename TObject>
class State_MapZone : public MapTag<TObject>
{
public:
    const ZoneObject* GetFirstZone() const
    {
        if (this->m_Map.empty())
        {
            return nullptr;
        }

        return &this->m_Map.begin()->second;
    }
};