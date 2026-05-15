#pragma once

#include <memory>

struct Core_State;
struct Core_System;
struct Core_UI;
struct Core_Hook_Domain;

// Input.
class Hook_GetButtonState;
class Hook_GetRawInputData;

// Lifecycle.
class Hook_GameEngineInit;
class Hook_EngineInitialize;
class Hook_DestroySubsystems;

// Render.
class Hook_ResizeBuffers;
class Hook_Present;

// Window.
class Hook_WndProc;

struct Core_Hook_Infrastructure
{
	Core_Hook_Infrastructure();
	~Core_Hook_Infrastructure();

	void Initialize(Core_State& state, Core_System& system, 
		Core_UI& ui, Core_Hook_Domain& hookDomain);
	void Shutdown();

	// Input.
	std::unique_ptr<Hook_GetButtonState> GetButtonState;
	std::unique_ptr<Hook_GetRawInputData> GetRawInputData;

	// Lifecycle.
	std::unique_ptr<Hook_GameEngineInit> GameEngineStart;
	std::unique_ptr<Hook_EngineInitialize> EngineInitialize;
	std::unique_ptr<Hook_DestroySubsystems> DestroySubsystems;

	// Render.
	std::unique_ptr<Hook_ResizeBuffers> ResizeBuffers;
	std::unique_ptr<Hook_Present> Present;

	// Window.
	std::unique_ptr<Hook_WndProc> WndProc;
};