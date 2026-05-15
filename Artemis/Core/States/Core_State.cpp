#include "pch.h"

// Header.
#include "Core_State.h"

#include "Domain/Core_State_Domain.h"
#include "Infrastructure/Core_State_Infrastructure.h"
#include "Interface/Core_State_Interface.h"

Core_State::Core_State() = default;
Core_State::~Core_State() = default;

void Core_State::Initialize()
{
	Domain = std::make_unique<Core_State_Domain>();
	Domain->Initialize();

	Infrastructure = std::make_unique<Core_State_Infrastructure>();
	Infrastructure->Initialize();

	Interface = std::make_unique<Core_State_Interface>();
	Interface->Initialize();
}

void Core_State::Shutdown() const
{
	Domain->Shutdown();
	Infrastructure->Shutdown();
	Interface->Shutdown();
}