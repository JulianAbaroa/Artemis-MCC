#include "pch.h"

// Header.
#include "Mod_Core.h"

// Cores.
#include "Core/States/Core_State.h"
#include "Core/Systems/Core_System.h"
#include "Core/Hooks/Core_Hook.h"
#include "Core/Threads/Core_Thread.h"
#include "Core/UI/Core_UI.h"

Mod_Core::Mod_Core() = default;
Mod_Core::~Mod_Core() = default;

// Note: The initialization order here is critical 
// for dependencies between subsystems.
void Mod_Core::Initialize()
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

void Mod_Core::Shutdown() const
{
    Thread->Shutdown();
    Hook->Shutdown();
    UI->Shutdown();
    System->Shutdown();
    State->Shutdown();
}