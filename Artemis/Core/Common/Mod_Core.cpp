#include "pch.h"
#include "Core/Common/Mod_Core.h"
#include "Core/States/Core_State.h"
#include "Core/Systems/Core_System.h"
#include "Core/Hooks/Core_Hook.h"
#include "Core/Threads/Core_Thread.h"
#include "Core/UI/Core_UI.h"

Core_State* g_pState = nullptr;
Core_System* g_pSystem = nullptr;
Core_Hook* g_pHook = nullptr;
Core_Thread* g_pThread = nullptr;
Core_UI* g_pUI = nullptr;

std::unique_ptr<Mod_Core> g_Mod = nullptr;

Mod_Core::Mod_Core()
{
    // Note: The initialization order here is critical 
    // for dependencies between subsystems.

    State = std::make_unique<Core_State>();
    g_pState = State.get();

    System = std::make_unique<Core_System>();
    g_pSystem = System.get();

    Hook = std::make_unique<Core_Hook>();
    g_pHook = Hook.get();

    Thread = std::make_unique<Core_Thread>();
    g_pThread = Thread.get();

    UI = std::make_unique<Core_UI>();
    g_pUI = UI.get();
}

Mod_Core::~Mod_Core()
{
	g_pState = nullptr;
	g_pSystem = nullptr;
	g_pHook = nullptr;
	g_pThread = nullptr;
	g_pUI = nullptr;
}