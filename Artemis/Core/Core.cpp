#include "pch.h"

#include "Core.h"

#include "Core/States/Core_State.h"
#include "Core/Systems/Core_System.h"
#include "Core/UI/Core_UI.h"
#include "Core/Hooks/Core_Hook.h"
#include "Core/Threads/Core_Thread.h"

Core::Core() = default;
Core::~Core() = default;

void Core::Initialize()
{
    State = std::make_unique<Core_State>();
    State->Initialize();

    System = std::make_unique<Core_System>();
    System->Initialize(*State);

    UI = std::make_unique<Core_UI>();
    UI->Initialize(*State, *System);

    Hook = std::make_unique<Core_Hook>();
    Hook->Initialize(*State, *System, *UI);

    Thread = std::make_unique<Core_Thread>();
    Thread->Initialize(*State, *System, *Hook);
}

void Core::Deinitialize() const
{
    Thread->Deinitialize();
    Hook->Deinitialize();
    UI->Deinitialize();
    System->Deinitialize();
    State->Deinitialize();
}