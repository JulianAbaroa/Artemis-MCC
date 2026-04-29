#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Input.h"
#include "Core/Systems/Infrastructure/Engine/System_Thread.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "Core/Threads/Infrastructure/Thread_Input.h"
#include <chrono>

using namespace std::chrono_literals;

void Thread_Input::Run()
{
    g_pSystem->Debug->Log("[InputThread] INFO: Started.");

    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        g_pSystem->Infrastructure->Thread->WaitOrExit(100ms);
    }

    g_pSystem->Debug->Log("[InputThread] INFO: Stopped.");
}