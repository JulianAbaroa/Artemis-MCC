#include "pch.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Interface/State_Debug.h"

Core_State::Core_State()
{
	Domain = std::make_unique<Core_State_Domain>();
	Infrastructure = std::make_unique<Core_State_Infrastructure>();
	Debug = std::make_unique<State_Debug>();
}

Core_State::~Core_State() = default;