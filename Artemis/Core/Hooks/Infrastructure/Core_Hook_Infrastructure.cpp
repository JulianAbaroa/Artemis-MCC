#include "pch.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"
#include "Core/Hooks/Infrastructure/Input/Hook_GetButtonState.h"
#include "Core/Hooks/Infrastructure/Input/Hook_GetRawInputData.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_GameEngineInit.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_EngineInitialize.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_DestroySubsystems.h"
#include "Core/Hooks/Infrastructure/Render/Hook_ResizeBuffers.h"
#include "Core/Hooks/Infrastructure/Render/Hook_Present.h"
#include "Core/Hooks/Infrastructure/Window/Hook_WndProc.h"

Core_Hook_Infrastructure::Core_Hook_Infrastructure()
{
	// Input.
	GetButtonState = std::make_unique<Hook_GetButtonState>();
	GetRawInputData = std::make_unique<Hook_GetRawInputData>();

	// Lifecycle.
	GameEngineStart = std::make_unique<Hook_GameEngineInit>();
	EngineInitialize = std::make_unique<Hook_EngineInitialize>();
	DestroySubsystems = std::make_unique<Hook_DestroySubsystems>();

	// Render.
	ResizeBuffers = std::make_unique<Hook_ResizeBuffers>();
	Present = std::make_unique<Hook_Present>();

	// Window.
	WndProc = std::make_unique<Hook_WndProc>();
}

Core_Hook_Infrastructure::~Core_Hook_Infrastructure() = default;