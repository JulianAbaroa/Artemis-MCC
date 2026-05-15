#include "pch.h"

// Header.
#include "Thread_Input.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Input/System_Input.h"
#include "Core/Systems/Infrastructure/Engine/Thread/System_Thread.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_Input::Run()
{
    m_Deps.System_Debug.Log("[InputThread] INFO: Started.");

    while (m_Deps.State_Lifecycle.IsRunning())
    {
        m_Deps.System_Thread.WaitOrExit(100ms);
    }

    m_Deps.System_Debug.Log("[InputThread] INFO: Stopped.");
}