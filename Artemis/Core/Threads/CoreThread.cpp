#include "pch.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Domain/MainThread.h"
#include "Core/Threads/Infrastructure/InputThread.h"
#include "Core/Threads/Domain/AIThread.h"

CoreThread::CoreThread()
{
	Main = std::make_unique<MainThread>();
	Input = std::make_unique<InputThread>();
	AI = std::make_unique<AIThread>();
}

CoreThread::~CoreThread() = default;