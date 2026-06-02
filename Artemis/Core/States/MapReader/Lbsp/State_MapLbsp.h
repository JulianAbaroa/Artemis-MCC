#pragma once

#include "Generated/Lbsp/LbspObject.h"

#include <unordered_map>
#include <string>
#include <mutex>

class State_MapLbsp
{
public:
    bool HasLbsp(const std::string& tagName) const;
    const LbspObject* GetLbsp(const std::string& tagName) const;
    void AddLbsp(const std::string& tagName, LbspObject zone);

    const LbspObject* GetFirstLbsp() const;
    const LbspObject* GetLbspBySbspIndex(int32_t sbspIndex) const;
    const std::unordered_map<std::string, LbspObject> GetAllLbsp() const;

    void Cleanup();

private:
    std::unordered_map<std::string, LbspObject> m_Lbsps;
    mutable std::mutex m_Mutex;
};