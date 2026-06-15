#include "pch.h"

#include "System_Input.h"

#include "Core/Types/Other/Input/InputTypes.h"

#include "Core/States/Other/Input/State_Input.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include <chrono>

using namespace std::chrono_literals;

void System_Input::AutomaticInput()
{
    InputRequest currentReq = { InputContext::Unknown, InputAction::Unknown };

    if (!m_Deps.State_Input.DequeueRequest(currentReq) ||
        currentReq.Action == InputAction::Unknown) return;
    
    // switch (currentReq.Action)
    // {
    // default:
    //     auto condition = []() { return false; };
    //     this->InjectInput(currentReq, condition, 100ms, 50ms);
    //     break;
    // }
}

bool System_Input::InjectInput(
    InputRequest request, 
    std::function<bool()> successCondition, 
    std::chrono::milliseconds timeoutMs,
    std::chrono::milliseconds stabilizeMs) 
{
	m_Deps.State_Input.SetNextRequest(request.Context, request.Action);

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

	m_Deps.State_Input.SetNextRequest(InputContext::Theater, InputAction::Unknown);

    if (stabilizeMs > 0ms) std::this_thread::sleep_for(stabilizeMs);

    m_Deps.State_Input.SetProcessing(false);

	return success;
}