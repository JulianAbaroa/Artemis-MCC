#include "pch.h"

// Header.
#include "Core_State_Interface.h"

#include "Debug/State_Debug.h"

Core_State_Interface::Core_State_Interface() = default;
Core_State_Interface::~Core_State_Interface() = default;

void Core_State_Interface::Initialize()
{
	Debug = std::make_unique<State_Debug>();
}

void Core_State_Interface::Shutdown()
{
	Debug.reset();
}