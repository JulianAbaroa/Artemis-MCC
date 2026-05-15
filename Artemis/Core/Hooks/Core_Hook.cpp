#include "pch.h"

// Header.
#include "Core_Hook.h"

#include "Core/States/Core_State.h"
#include "Core/Systems/Core_System.h"
#include "Core/UI/Core_UI.h"

#include "Core/Hooks/Domain/Core_Hook_Domain.h"

#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"

Core_Hook::Core_Hook() = default;
Core_Hook::~Core_Hook() = default;

void Core_Hook::Initialize(Core_State& state, 
	Core_System& system, Core_UI& ui)
{
	Domain = std::make_unique<Core_Hook_Domain>();
	Domain->Initialize(state, system);

	Infrastructure = std::make_unique<Core_Hook_Infrastructure>();
	Infrastructure->Initialize(state, system, ui, *Domain);
}

void Core_Hook::Shutdown() const
{
	Domain->Shutdown();
	Infrastructure->Shutdown();
}