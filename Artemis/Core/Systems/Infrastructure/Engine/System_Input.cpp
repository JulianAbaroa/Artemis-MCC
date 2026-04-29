#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Input.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Input.h"
#include "Core/Systems/Interface/System_Debug.h"
#include <chrono>

using namespace std::chrono_literals;

void System_Input::AutomaticInput()
{
    InputRequest currentReq = { InputContext::Unknown, InputAction::Unknown };

    if (!g_pState->Infrastructure->Input->DequeueRequest(currentReq) ||
        currentReq.Action == InputAction::Unknown) return;
    
    switch (currentReq.Action)
    {
    default:
        auto condition = []() { return false; };
        this->InjectInput(currentReq, condition, 100ms, 50ms);
        break;
    }
}

bool System_Input::InjectInput(
    InputRequest request, 
    std::function<bool()> successCondition, 
    std::chrono::milliseconds timeoutMs,
    std::chrono::milliseconds stabilizeMs) 
{
	g_pState->Infrastructure->Input->SetNextRequest(request.Context, request.Action);

	auto startWait = std::chrono::steady_clock::now();
	bool success = false;

	while (std::chrono::steady_clock::now() - startWait < timeoutMs)
	{
		if (successCondition())
		{
			success = true;
			break;
		}

		std::this_thread::yield();
	}

	g_pState->Infrastructure->Input->SetNextRequest(InputContext::Theater, InputAction::Unknown);

    if (stabilizeMs > 0ms) std::this_thread::sleep_for(stabilizeMs);

	g_pState->Infrastructure->Input->SetProcessing(false);

	return success;
}