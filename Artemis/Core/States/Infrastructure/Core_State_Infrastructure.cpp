#include "pch.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Input.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"
#include "Core/States/Infrastructure/Engine/State_Render.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"

Core_State_Infrastructure::Core_State_Infrastructure()
{
	Input = std::make_unique<State_Input>();
	Lifecycle = std::make_unique<State_Lifecycle>();
	Render = std::make_unique<State_Render>();
	Settings = std::make_unique<State_Settings>();
}

Core_State_Infrastructure::~Core_State_Infrastructure() = default;