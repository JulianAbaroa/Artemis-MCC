#pragma once

#include "Generated/Eqip/EqipObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class State_MapEqip
{
public:
    bool HasEqip(const std::string& tagName) const;
    const EqipObject* GetEqip(const std::string& tagName) const;
    void AddEqip(const std::string& tagName, EqipObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, EqipObject> m_Eqips;
    mutable std::mutex m_Mutex;
};