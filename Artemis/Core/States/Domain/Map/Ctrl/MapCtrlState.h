#pragma once

#include "Generated/Ctrl/CtrlObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class MapCtrlState
{
public:
    bool HasCtrl(const std::string& tagName) const;
    const CtrlObject* GetCtrl(const std::string& tagName) const;
    void AddCtrl(const std::string& tagName, CtrlObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, CtrlObject> m_Ctrls;
    mutable std::mutex m_Mutex;
};