#pragma once

#include "Generated/Bloc/BlocObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class State_MapBloc
{
public:
    bool HasBloc(const std::string& tagName) const;
    const BlocObject* GetBloc(const std::string& tagName) const;
    void AddBloc(const std::string& tagName, BlocObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, BlocObject> m_Blocs;
    mutable std::mutex m_Mutex;
};