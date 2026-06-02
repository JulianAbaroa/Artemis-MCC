#pragma once

#include <memory>

class Core_State;
class Core_System;
class Core_UI;

// Related to map.
class Hook_BlamOpenMap;

// Related to gametype.
class Hook_GameEngineInit;

// Related to object table.
class Hook_ObjectTable;
class Hook_CreateObject;
class Hook_ReleaseObject;

// Related to player table.
class Hook_CreatePlayer;
class Hook_PlayerTable;

// Related to interaction table.
class Hook_InteractionTable;

// Related to input.
class Hook_GetButtonState;
class Hook_GetRawInputData;

// Related to game engine initialization/deinitialization.
class Hook_EngineInitialize;
class Hook_DestroySubsystems;

// Related to window.
class Hook_WndProc;

// Related to render.
class Hook_ResizeBuffers;
class Hook_Present;

class Core_Hook	
{
public:
	Core_Hook();
	~Core_Hook();

	void Initialize(Core_State& state, 
		Core_System& system, Core_UI& ui);
	void Deinitialize();

	std::unique_ptr<Hook_BlamOpenMap> BlamOpenMap;

	std::unique_ptr<Hook_GameEngineInit> GameEngineInit;

	std::unique_ptr<Hook_ObjectTable> ObjectTable;
	std::unique_ptr<Hook_CreateObject> CreateGameObject;
	std::unique_ptr<Hook_ReleaseObject> ReleaseGameObject;

	std::unique_ptr<Hook_CreatePlayer> CreatePlayer;
	std::unique_ptr<Hook_PlayerTable> PlayerTable;

	std::unique_ptr<Hook_InteractionTable> InteractionTable;

	std::unique_ptr<Hook_GetButtonState> GetButtonState;
	std::unique_ptr<Hook_GetRawInputData> GetRawInputData;

	std::unique_ptr<Hook_EngineInitialize> EngineInitialize;
	std::unique_ptr<Hook_DestroySubsystems> DestroySubsystems;

	std::unique_ptr<Hook_WndProc> WndProc;

	std::unique_ptr<Hook_ResizeBuffers> ResizeBuffers;
	std::unique_ptr<Hook_Present> Present;

private:
	void InitMap(Core_State& state, Core_System& system);
	void DeinitMap();

	void InitGametype(Core_System& system);
	void DeinitGametype();

	void InitObject(Core_State& state, Core_System& system);
	void DeinitObject();

	void InitPlayer(Core_State& state, Core_System& system);
	void DeinitPlayer();

	void InitInteraction(Core_State& state, Core_System& system);
	void DeinitInteraction();

	void InitInput(Core_State& state, Core_System& system, Core_UI& ui);
	void DeinitInput();

	void InitLifecycle(Core_State& state, Core_System& system, Core_UI& ui);
	void DeinitLifecycle();

	void InitWindow(Core_State& state, Core_System& system, Core_UI& ui);
	void DeinitWindow();

	void InitRender(Core_State& state, Core_System& system, Core_UI& ui);
	void DeinitRender();
};