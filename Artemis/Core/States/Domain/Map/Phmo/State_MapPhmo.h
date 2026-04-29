#pragma once

#include "Generated/Phmo/PhmoObject.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>

// Stores all physics models parsed from the current map's phmo tags.
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
    // Keyed by hlmt TagName (e.g. "objects\characters\spartans\spartans").
    std::unordered_map<std::string, PhmoObject> m_Phmos;
    mutable std::mutex m_Mutex;
};