#pragma once

#include "../MapTag.h"

#include "Generated/Hlmt/HlmtObject.h"

#include <vector>

template <typename TObject>
class State_MapHlmt : public MapTag<TObject>
{
public:
    std::vector<std::string> GetTagNames() const 
    {
        std::vector<std::string> names;
        names.reserve(this->m_Map.size());

        for (const auto& kv : this->m_Map)
        {
            names.push_back(kv.first);
        }

        return names;
    }
};