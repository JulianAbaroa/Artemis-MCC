#pragma once

#include "Generated/Sbsp/SbspObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class MapSbspState
{
public:
    bool HasSbsp(const std::string& tagName) const;
    const SbspObject* GetSbsp(const std::string& tagName) const;
    void AddSbsp(const std::string& tagName, SbspObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, SbspObject> m_Sbsps;
    mutable std::mutex m_Mutex;
};