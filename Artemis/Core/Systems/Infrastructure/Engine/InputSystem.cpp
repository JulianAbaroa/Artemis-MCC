#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/InputState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/InputSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <chrono>

using namespace std::chrono_literals;

void InputSystem::AutomaticInput()
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

bool InputSystem::InjectInput(
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