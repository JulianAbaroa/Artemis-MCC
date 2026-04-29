#pragma once

#include "Generated/Coll/CollObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class State_MapColl
{
public:
    bool HasColl(const std::string& tagName) const;
    const CollObject* GetColl(const std::string& tagName) const;
    void AddColl(const std::string& tagName, CollObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, CollObject> m_Colls;
    mutable std::mutex m_Mutex;
};