#include "pch.h"

#include "Thread_Input.h"

#include "Core/States/Lifecycle/State_Lifecycle.h"
#include "Core/States/Settings/State_Settings.h"

#include "Core/Systems/Input/System_Input.h"
#include "Core/Systems/Logs/System_Logs.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_Input::Run()
{
    m_Deps.System_Logs.Log("[InputThread] INFO: Started.");

    while (m_Deps.State_Lifecycle.IsRunning()) { }

    m_Deps.System_Logs.Log("[InputThread] INFO: Stopped.");
}