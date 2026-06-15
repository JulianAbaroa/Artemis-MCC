#pragma once

#include "../MapTag.h"

#include "Generated/Play/PlayObject.h"

template <typename TObject>
class State_MapPlay : public MapTag<TObject>
{
public:
    const PlayObject* GetFirstPlay() const
    {
        if (this->m_Map.empty())
        {
            return nullptr;
        }

        return &this->m_Map.begin()->second;
    }
};