#pragma once

#include <functional>
#include <chrono>
#include <map>

struct InputRequest;

class State_Input;
class System_Logs;

struct Sys_Input_Deps
{
    State_Input& State_Input;
    System_Logs& System_Logs;
};

class System_Input
{
public:
    System_Input(Sys_Input_Deps deps) : m_Deps(deps) {}
    ~System_Input() = default;

    void AutomaticInput();
    
private:
    Sys_Input_Deps m_Deps;

	bool InjectInput(
        InputRequest req, 
        std::function<bool()> successCondition, 
        std::chrono::milliseconds timeoutMs, 
        std::chrono::milliseconds stabilizeMs
    );
};