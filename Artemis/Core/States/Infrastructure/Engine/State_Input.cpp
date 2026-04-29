#include "pch.h"
#include "Core/States/Infrastructure/Engine/State_Input.h"

// We don't use a mutex here because the HookedGetButtonState() is very sensitive.
InputRequest State_Input::GetNextRequest() const { return m_NextRequest; }
bool State_Input::IsProcessing() const { return m_IsProcessing.load(); }

void State_Input::SetNextRequest(InputContext context, InputAction action)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_IsProcessing.store(true);
	m_NextRequest = { context, action };
}

void State_Input::SetProcessing(bool processing) { m_IsProcessing.store(processing); }


void State_Input::EnqueueRequest(const InputRequest& request, bool uniqueRequest)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (uniqueRequest && !m_Queue.empty())
	{
		if (m_Queue.back().Action == request.Action) return;
	}

	m_Queue.push(request);
}

bool State_Input::DequeueRequest(InputRequest& outRequest)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Queue.empty()) return false;
	outRequest = m_Queue.front();
	m_Queue.pop();
	return true;
}

void State_Input::Cleanup()
{
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		std::queue<InputRequest> empty;
		std::swap(m_Queue, empty);

		m_NextRequest = { InputContext::Unknown, InputAction::Unknown };
	}

	m_IsProcessing.store(false);
}