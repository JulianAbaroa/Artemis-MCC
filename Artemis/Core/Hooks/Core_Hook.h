#pragma once

#include <memory>

class Core_State;
class Core_System;
class Core_UI;

// --- Layer 0: Sources ---
class Hook_BlamOpenMap; class Hook_ObjectTable;
class Hook_InitRootNode; class Hook_CreateObject;
class Hook_ReleaseObject; class Hook_CreatePlayer;
class Hook_PlayerTable; class Hook_InteractionTable;

// Tick.
class Hook_SimulationTicks;

// Other.
class Hook_GetButtonState; class Hook_GetRawInputData;
class Hook_EngineInitialize; class Hook_DestroySubsystems;
class Hook_ResizeBuffers; class Hook_Present;
class Hook_WndProc;

class Core_Hook	
{
public:
	Core_Hook();
	~Core_Hook();

	void Initialize(Core_State& state, 
		Core_System& system, Core_UI& ui);
	void Deinitialize();

	// --- Layer 0: Sources ---
	std::unique_ptr<Hook_BlamOpenMap> BlamOpenMap;
	std::unique_ptr<Hook_ObjectTable> ObjectTable;
	std::unique_ptr<Hook_InitRootNode> InitRootNode;
	std::unique_ptr<Hook_CreateObject> CreateGameObject;
	std::unique_ptr<Hook_ReleaseObject> ReleaseGameObject;
	std::unique_ptr<Hook_CreatePlayer> CreatePlayer;
	std::unique_ptr<Hook_PlayerTable> PlayerTable;
	std::unique_ptr<Hook_InteractionTable> InteractionTable;

	// Tick.
	std::unique_ptr<Hook_SimulationTicks> SimulationTicks;

	// Other.
	std::unique_ptr<Hook_GetButtonState> GetButtonState;
	std::unique_ptr<Hook_GetRawInputData> GetRawInputData;
	std::unique_ptr<Hook_EngineInitialize> EngineInitialize;
	std::unique_ptr<Hook_DestroySubsystems> DestroySubsystems;
	std::unique_ptr<Hook_ResizeBuffers> ResizeBuffers;
	std::unique_ptr<Hook_Present> Present;
	std::unique_ptr<Hook_WndProc> WndProc;

private:
	void InitSources(Core_State& state, Core_System& system);
	void DeinitSources();

	void InitTick(Core_State& state, Core_System& system, Core_UI& ui);
	void DeinitTick();

	void InitOther(Core_State& state, Core_System& system, Core_UI& ui);
	void DeinitOther();
};