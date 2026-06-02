#pragma once

#include "Generated/Play/PlayObject.h"

#include <unordered_map>
#include <string>
#include <mutex>

class State_MapPlay
{
public:
    bool HasPlay(const std::string& tagName) const;
    const PlayObject* GetPlay(const std::string& tagName) const;
    void AddPlay(const std::string& tagName, PlayObject zone);
    const PlayObject* GetFirstPlay() const;
    void Cleanup();

private:
    std::unordered_map<std::string, PlayObject> m_Plays;
    mutable std::mutex m_Mutex;
};