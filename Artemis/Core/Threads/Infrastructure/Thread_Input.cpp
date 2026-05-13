#include "pch.h"

// Header.
#include "Thread_Input.h"

// --- States. ---

#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

#include "Core/States/Infrastructure/Persistence/State_Settings.h"

// --- Systems ---

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Infrastructure/Engine/Input/System_Input.h"
#include "Core/Systems/Infrastructure/Engine/Thread/System_Thread.h"

#include "Core/Systems/Interface/System_Debug.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_Input::Run()
{
    auto& debug = *g_pSystem->Debug;
    debug.Log("[InputThread] INFO: Started.");

    auto& thread = *g_pSystem->Infrastructure->Thread;
    auto& lifecycle = *g_pState->Infrastructure->Lifecycle;
    while (lifecycle.IsRunning())
    {
        thread.WaitOrExit(100ms);
    }

    debug.Log("[InputThread] INFO: Stopped.");
}