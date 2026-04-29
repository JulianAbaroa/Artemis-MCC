#include "pch.h"
#include "Core/Threads/Core_Thread.h"
#include "Core/Threads/Domain/Thread_Main.h"
#include "Core/Threads/Infrastructure/Thread_Input.h"
#include "Core/Threads/Domain/Thread_AI.h"

Core_Thread::Core_Thread()
{
	Main = std::make_unique<Thread_Main>();
	Input = std::make_unique<Thread_Input>();
	AI = std::make_unique<Thread_AI>();
}

Core_Thread::~Core_Thread() = default;