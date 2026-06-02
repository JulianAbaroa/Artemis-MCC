#pragma once

#include <functional>
#include <chrono>
#include <map>

struct InputRequest;

class State_Input;
class System_Logs;

struct System_Input_Dependencies
{
    State_Input& State_Input;
    System_Logs& System_Logs;
};

class System_Input
{
public:
    System_Input(System_Input_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_Input() = default;

    void AutomaticInput();
    
private:
    System_Input_Dependencies m_Deps;

    const std::map<int, float> INPUT_SPEED_MAP = {
        {'0', 16.0f},  {'9', 8.0f},   {'8', 4.0f},
        {'7', 1.0f},   {'6', 0.5f},   {'5', 0.25f},
        {'4', 0.1f},   {'3', 0.0f},
    };

	bool InjectInput(
        InputRequest req, 
        std::function<bool()> successCondition, 
        std::chrono::milliseconds timeoutMs, 
        std::chrono::milliseconds stabilizeMs
    );
};