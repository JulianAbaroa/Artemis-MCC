#pragma once

#include <memory>

struct Core_State;
struct Core_System;

// Map.
class Hook_BlamOpenMap;

// Object.
class Hook_CreateObject;
class Hook_ReleaseObject;
class Hook_ObjectTable;

// Player.
class Hook_CreatePlayer;
class Hook_PlayerTable;

// Interaction.
class Hook_InteractionTable;

struct Core_Hook_Domain
{
	Core_Hook_Domain();
	~Core_Hook_Domain();

	void Initialize(Core_State& state, Core_System& system);
	void Shutdown();

	// Map.
	std::unique_ptr<Hook_BlamOpenMap> BlamOpenMap;

	// Object.
	std::unique_ptr<Hook_CreateObject> CreateGameObject;
	std::unique_ptr<Hook_ReleaseObject> ReleaseGameObject;
	std::unique_ptr<Hook_ObjectTable> ObjectTable;

	// Player.
	std::unique_ptr<Hook_CreatePlayer> CreatePlayer;
	std::unique_ptr<Hook_PlayerTable> PlayerTable;

	// Interaction.
	std::unique_ptr<Hook_InteractionTable> InteractionTable;
};