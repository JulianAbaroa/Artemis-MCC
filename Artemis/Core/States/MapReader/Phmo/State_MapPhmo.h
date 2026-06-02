#pragma once

#include "Generated/Phmo/PhmoObject.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>

class State_MapPhmo
{
public:
    bool HasPhmo(const std::string& tagName) const;
    const PhmoObject* GetPhmo(const std::string& tagName) const;
    void AddPhmo(const std::string& tagName, PhmoObject data);

    size_t GetSize() const;

    std::vector<std::string> GetTagNames();

    void Cleanup();

private:
    std::unordered_map<std::string, PhmoObject> m_Phmos;
    mutable std::mutex m_Mutex;
};