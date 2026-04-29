#pragma once

#include <memory>

// Map.
class Hook_BlamOpenMap;

// Tables.
class Hook_CreateObject;
class Hook_ReleaseObject;
class Hook_CreatePlayer;
class Hook_PlayerTable;
class Hook_ObjectTable;
class Hook_InteractionTable;

// Legacy.
//class BlamOpenFileHook;
class Hook_BuildGameEvent;

struct Core_Hook_Domain
{
	Core_Hook_Domain();
	~Core_Hook_Domain();

	// Map.
	std::unique_ptr<Hook_BlamOpenMap> BlamOpenMap;

	// Tables.
	std::unique_ptr<Hook_CreateObject> CreateGO;
	std::unique_ptr<Hook_ReleaseObject> ReleaseGO;
	std::unique_ptr<Hook_CreatePlayer> CreatePlayer;
	std::unique_ptr<Hook_PlayerTable> PlayerTable;
	std::unique_ptr<Hook_ObjectTable> ObjectTable;
	std::unique_ptr<Hook_InteractionTable> InteractionTable;

	// Legacy.
	//std::unique_ptr<BlamOpenFileHook> BlamOpenFile;
	std::unique_ptr<Hook_BuildGameEvent> BuildGameEvent;
};